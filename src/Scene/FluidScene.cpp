#include "FluidScene.h"

FluidScene::FluidScene(DXContext* context, 
                       RenderPipeline* pipeline, 
                       ComputePipeline* bilevelUniformGridCP, 
                       ComputePipeline* surfaceBlockDetectionCP,
                       ComputePipeline* surfaceCellDetectionCP,
                       ComputePipeline* surfaceVertexCompactionCP,
                       ComputePipeline* surfaceVertexDensityCP,
                       ComputePipeline* surfaceVertexNormalCP,
                       MeshPipeline* fluidMeshPipeline)
    : Drawable(context, pipeline), 
      bilevelUniformGridCP(bilevelUniformGridCP), 
      surfaceBlockDetectionCP(surfaceBlockDetectionCP),
      surfaceCellDetectionCP(surfaceCellDetectionCP),
      surfaceVertexCompactionCP(surfaceVertexCompactionCP),
      surfaceVertexDensityCP(surfaceVertexDensityCP),
      surfaceVertexNormalCP(surfaceVertexNormalCP),
      fluidMeshPipeline(fluidMeshPipeline)
{
    constructScene();
}

// In this pipeline, drawing is done via a mesh shader
void FluidScene::draw(Camera* camera) {
    auto cmdList = fluidMeshPipeline->getCommandList();
    MeshShadingConstants meshShadingConstants = { camera->getViewProjMat(), gridConstants.gridDim, gridConstants.resolution, gridConstants.minBounds };

    cmdList->SetPipelineState(fluidMeshPipeline->getPSO());
    cmdList->SetComputeRootSignature(fluidMeshPipeline->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { fluidMeshPipeline->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Transition surfaceHalfBlockDispatch to an SRV
    D3D12_RESOURCE_BARRIER surfaceHalfBlockDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceHalfBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition surfaceVertexNormalBuffer to an SRV
    D3D12_RESOURCE_BARRIER surfaceVertexNormalBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertexNormalBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    D3D12_RESOURCE_BARRIER barriers[2] = { surfaceHalfBlockDispatchBarrier, surfaceVertexNormalBufferBarrier };
    cmdList->ResourceBarrier(2, barriers);

    // Set graphics root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, surfaceBlockIndicesBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, surfaceVertDensityBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(2, surfaceVertexNormalBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootShaderResourceView(3, surfaceHalfBlockDispatch.getGPUVirtualAddress());
    cmdList->SetComputeRoot32BitConstants(4, 23, &meshShadingConstants, 0);

    // Transition surfaceHalfBlockDispatch to indirect argument buffer
    D3D12_RESOURCE_BARRIER surfaceHalfBlockDispatchBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceHalfBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
    );

    cmdList->ResourceBarrier(1, &surfaceHalfBlockDispatchBarrier2);

    // Draws
    cmdList->ExecuteIndirect(commandSignature, 1, surfaceHalfBlockDispatch.getBuffer(), 0, nullptr, 0);

    transitionBuffersToUAV(cmdList);

    context->executeCommandList(fluidMeshPipeline->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(fluidMeshPipeline->getCommandListID());
}

float getRandomFloatInRange(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

void FluidScene::constructScene() {
    unsigned int numParticles = 3 * CELLS_PER_BLOCK_EDGE * 3 * CELLS_PER_BLOCK_EDGE * 3 * CELLS_PER_BLOCK_EDGE;
    gridConstants = { numParticles, {3 * CELLS_PER_BLOCK_EDGE, 3 * CELLS_PER_BLOCK_EDGE, 3 * CELLS_PER_BLOCK_EDGE}, {0.f, 0.f, 0.f}, 0.1f };

    // Populate position data. 1000 partices in a 12x12x12 block of cells, each at a random position in a cell.
    // (Temporary, eventually, position data will come from simulation)
    for (unsigned int i = 0; i < gridConstants.gridDim.x; ++i) {
        for (unsigned int j = 0; j < gridConstants.gridDim.y; ++j) {
            for (unsigned int k = 0; k < gridConstants.gridDim.z; ++k) {
                positions.push_back({ 
                    gridConstants.minBounds.x + gridConstants.resolution * i + getRandomFloatInRange(0.f, gridConstants.resolution),
                    gridConstants.minBounds.y + gridConstants.resolution * j + getRandomFloatInRange(0.f, gridConstants.resolution),
                    gridConstants.minBounds.z + gridConstants.resolution * k + getRandomFloatInRange(0.f, gridConstants.resolution) 
                });
            }
        }
    }

    positionBuffer = StructuredBuffer(positions.data(), gridConstants.numParticles, sizeof(XMFLOAT3));
    positionBuffer.passDataToGPU(*context, bilevelUniformGridCP->getCommandList(), bilevelUniformGridCP->getCommandListID());
    positionBuffer.createSRV(*context, bilevelUniformGridCP->getDescriptorHeap());

    // Create cells and blocks buffers
    int numCells = gridConstants.gridDim.x * gridConstants.gridDim.y * gridConstants.gridDim.z;
    int numBlocks = numCells / (CELLS_PER_BLOCK_EDGE * CELLS_PER_BLOCK_EDGE * CELLS_PER_BLOCK_EDGE);
    int numVerts = (gridConstants.gridDim.x + 1) * (gridConstants.gridDim.y + 1) * (gridConstants.gridDim.z + 1);
    
    std::vector<Cell> cells(numCells);
    std::vector<Block> blocks(numBlocks);
    std::vector<unsigned int> surfaceBlockIndices(numBlocks, 0);
    XMUINT3 dipatchCPU = { 0, 0, 0 };
    std::vector<unsigned int> surfaceVertices(numVerts, 0);
    std::vector<unsigned int> surfaceVertexIndices(numVerts, 0);
    std::vector<float> surfaceVertexDensities(numVerts, 0.f);
    std::vector<XMFLOAT2> surfaceVertexNormals(numVerts, { 0.f, 0.f }); // (x, y) components of the normal; z component can be inferred. This helps save space.

    blocksBuffer = StructuredBuffer(blocks.data(), numBlocks, sizeof(Block));
    blocksBuffer.passDataToGPU(*context, bilevelUniformGridCP->getCommandList(), bilevelUniformGridCP->getCommandListID());
    blocksBuffer.createUAV(*context, bilevelUniformGridCP->getDescriptorHeap());

    cellsBuffer = StructuredBuffer(cells.data(), numCells, sizeof(Cell));
    cellsBuffer.passDataToGPU(*context, bilevelUniformGridCP->getCommandList(), bilevelUniformGridCP->getCommandListID());
    cellsBuffer.createUAV(*context, bilevelUniformGridCP->getDescriptorHeap());

    surfaceBlockIndicesBuffer = StructuredBuffer(surfaceBlockIndices.data(), numBlocks, sizeof(unsigned int));
    surfaceBlockIndicesBuffer.passDataToGPU(*context, surfaceBlockDetectionCP->getCommandList(), surfaceBlockDetectionCP->getCommandListID());
    surfaceBlockIndicesBuffer.createUAV(*context, surfaceBlockDetectionCP->getDescriptorHeap());

    surfaceBlockDispatch = StructuredBuffer(&dipatchCPU, 1, sizeof(XMUINT3));
    surfaceBlockDispatch.passDataToGPU(*context, surfaceBlockDetectionCP->getCommandList(), surfaceBlockDetectionCP->getCommandListID());
    surfaceBlockDispatch.createUAV(*context, surfaceBlockDetectionCP->getDescriptorHeap());

    surfaceHalfBlockDispatch = StructuredBuffer(&dipatchCPU, 1, sizeof(XMUINT3));
    surfaceHalfBlockDispatch.passDataToGPU(*context, surfaceBlockDetectionCP->getCommandList(), surfaceCellDetectionCP->getCommandListID());
    surfaceHalfBlockDispatch.createUAV(*context, surfaceBlockDetectionCP->getDescriptorHeap());

    surfaceVerticesBuffer = StructuredBuffer(surfaceVertices.data(), numVerts, sizeof(unsigned int));
    surfaceVerticesBuffer.passDataToGPU(*context, surfaceCellDetectionCP->getCommandList(), surfaceCellDetectionCP->getCommandListID());
    surfaceVerticesBuffer.createUAV(*context, surfaceCellDetectionCP->getDescriptorHeap());

    surfaceVertexIndicesBuffer = StructuredBuffer(surfaceVertexIndices.data(), numVerts, sizeof(unsigned int));
    surfaceVertexIndicesBuffer.passDataToGPU(*context, surfaceVertexCompactionCP->getCommandList(), surfaceVertexCompactionCP->getCommandListID());
    surfaceVertexIndicesBuffer.createUAV(*context, surfaceVertexCompactionCP->getDescriptorHeap());

    surfaceVertDensityDispatch = StructuredBuffer(&dipatchCPU, 1, sizeof(XMUINT3));
    surfaceVertDensityDispatch.passDataToGPU(*context, surfaceVertexCompactionCP->getCommandList(), surfaceVertexCompactionCP->getCommandListID());
    surfaceVertDensityDispatch.createUAV(*context, surfaceVertexCompactionCP->getDescriptorHeap());

    surfaceVertDensityBuffer = StructuredBuffer(surfaceVertexDensities.data(), numVerts, sizeof(float));
    surfaceVertDensityBuffer.passDataToGPU(*context, surfaceVertexDensityCP->getCommandList(), surfaceVertexDensityCP->getCommandListID());
    surfaceVertDensityBuffer.createUAV(*context, surfaceVertexDensityCP->getDescriptorHeap());

    surfaceVertexNormalBuffer = StructuredBuffer(surfaceVertexNormals.data(), numVerts, sizeof(XMFLOAT2));
    surfaceVertexNormalBuffer.passDataToGPU(*context, surfaceVertexNormalCP->getCommandList(), surfaceVertexNormalCP->getCommandListID());
    surfaceVertexNormalBuffer.createUAV(*context, surfaceVertexNormalCP->getDescriptorHeap());

	// Create Command Signature
	// Describe the arguments for an indirect dispatch
	D3D12_INDIRECT_ARGUMENT_DESC argumentDesc = {};
	argumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

	// Command signature description
	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	commandSignatureDesc.ByteStride = sizeof(XMUINT3); // Argument buffer stride
	commandSignatureDesc.NumArgumentDescs = 1; // One argument descriptor
	commandSignatureDesc.pArgumentDescs = &argumentDesc;

	// Create the command signature
	context->getDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&commandSignature));

    // Create fence
    context->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
}

void FluidScene::compute() {
    // TODO: do a reset compute pass first to clear the buffers (take advantage of existing passes where possible)
    // (This is a todo, because I need to implement a third compute pass that operates on the cell level to clear the cell buffers)
    computeBilevelUniformGrid();
    computeSurfaceBlockDetection();
    computeSurfaceCellDetection();
    compactSurfaceVertices();
    computeSurfaceVertexDensity();
    computeSurfaceVertexNormal();
}

void FluidScene::computeBilevelUniformGrid() {
    auto cmdList = bilevelUniformGridCP->getCommandList();

    cmdList->SetPipelineState(bilevelUniformGridCP->getPSO());
    cmdList->SetComputeRootSignature(bilevelUniformGridCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { bilevelUniformGridCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, positionBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, cellsBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(2, blocksBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstants(3, 8, &gridConstants, 0);

    // Dispatch
    int numWorkGroups = (gridConstants.numParticles + BILEVEL_UNIFORM_GRID_THREADS_X - 1) / BILEVEL_UNIFORM_GRID_THREADS_X;
    cmdList->Dispatch(numWorkGroups, 1, 1);

    // Transition blocksBuffer from UAV to SRV for the next pass
    D3D12_RESOURCE_BARRIER blocksBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        blocksBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    cmdList->ResourceBarrier(1, &blocksBufferBarrier);

    // Execute command list
    context->executeCommandList(bilevelUniformGridCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    // Reinitialize command list
    context->resetCommandList(bilevelUniformGridCP->getCommandListID());
}

void FluidScene::computeSurfaceBlockDetection() {
    auto cmdList = surfaceBlockDetectionCP->getCommandList();

    cmdList->SetPipelineState(surfaceBlockDetectionCP->getPSO());
    cmdList->SetComputeRootSignature(surfaceBlockDetectionCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { surfaceBlockDetectionCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);

    int numCells = gridConstants.gridDim.x * gridConstants.gridDim.y * gridConstants.gridDim.z;
    int numBlocks = numCells / (CELLS_PER_BLOCK_EDGE * CELLS_PER_BLOCK_EDGE * CELLS_PER_BLOCK_EDGE);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, blocksBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, surfaceBlockIndicesBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootUnorderedAccessView(2, surfaceBlockDispatch.getGPUVirtualAddress());

    cmdList->SetComputeRoot32BitConstants(3, 1, &numBlocks, 0);

    // Dispatch
    int numWorkGroups = (numBlocks + SURFACE_BLOCK_DETECTION_THREADS_X - 1) / SURFACE_BLOCK_DETECTION_THREADS_X;
    cmdList->Dispatch(numWorkGroups, 1, 1);

    context->executeCommandList(surfaceBlockDetectionCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(surfaceBlockDetectionCP->getCommandListID());
}

void FluidScene::computeSurfaceCellDetection() {
    auto cmdList = surfaceCellDetectionCP->getCommandList();

    cmdList->SetPipelineState(surfaceCellDetectionCP->getPSO());
    cmdList->SetComputeRootSignature(surfaceCellDetectionCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { surfaceCellDetectionCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);

    // Transition surfaceBlockIndicesBuffer to SRV 
    D3D12_RESOURCE_BARRIER surfaceBlockIndicesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockIndicesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition surfaceBlockDispatch to an SRV
    D3D12_RESOURCE_BARRIER surfaceBlockDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition cellsBuffer to SRV 
    D3D12_RESOURCE_BARRIER cellsBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        cellsBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    D3D12_RESOURCE_BARRIER barriers[3] = { surfaceBlockIndicesBufferBarrier, surfaceBlockDispatchBarrier, cellsBufferBarrier };
    cmdList->ResourceBarrier(1, barriers);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, surfaceBlockIndicesBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, cellsBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(2, surfaceVerticesBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRootShaderResourceView(3, surfaceBlockDispatch.getGPUVirtualAddress());
    cmdList->SetComputeRootUnorderedAccessView(4, surfaceHalfBlockDispatch.getGPUVirtualAddress());
    cmdList->SetComputeRoot32BitConstants(5, 3, &gridConstants.gridDim, 0);

    // Transition surfaceBlockDispatch to indirect argument buffer
    D3D12_RESOURCE_BARRIER surfaceBlockDispatchBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
    );

    cmdList->ResourceBarrier(1, &surfaceBlockDispatchBarrier2);

    // Dispatch
    cmdList->ExecuteIndirect(commandSignature, 1, surfaceBlockDispatch.getBuffer(), 0, nullptr, 0);

    context->executeCommandList(surfaceCellDetectionCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(surfaceCellDetectionCP->getCommandListID());
}

void FluidScene::compactSurfaceVertices() {
    auto cmdList = surfaceVertexCompactionCP->getCommandList();

    cmdList->SetPipelineState(surfaceVertexCompactionCP->getPSO());
    cmdList->SetComputeRootSignature(surfaceVertexCompactionCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { surfaceVertexCompactionCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);


    // Transition surfaceBlockDispatch to an SRV 
    D3D12_RESOURCE_BARRIER surfaceBlockDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition surfaceVerticesBuffer back to SRV 
    D3D12_RESOURCE_BARRIER surfaceVerticesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVerticesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE 
    );

    D3D12_RESOURCE_BARRIER barriers[3] = { surfaceBlockDispatchBarrier, surfaceVerticesBufferBarrier };
    cmdList->ResourceBarrier(1, barriers);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, surfaceVerticesBuffer.getSRVGPUDescriptorHandle());
    //make sure this should be block and SRV
    cmdList->SetComputeRootDescriptorTable(1, surfaceBlockDispatch.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(2, surfaceVertexIndicesBuffer.getUAVGPUDescriptorHandle());

    cmdList->SetComputeRootUnorderedAccessView(3, surfaceVertDensityDispatch.getGPUVirtualAddress());

    // Transition surfaceBlockDispatch to indirect argument buffer
    D3D12_RESOURCE_BARRIER surfaceBlockDispatchBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
    );

    cmdList->ResourceBarrier(1, &surfaceBlockDispatchBarrier2);

    // Dispatch
    cmdList->ExecuteIndirect(commandSignature, 1, surfaceBlockDispatch.getBuffer(), 0, nullptr, 0);

    context->executeCommandList(surfaceVertexCompactionCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(surfaceVertexCompactionCP->getCommandListID());
}

void FluidScene::computeSurfaceVertexDensity() {
    auto cmdList = surfaceVertexDensityCP->getCommandList();

    cmdList->SetPipelineState(surfaceVertexDensityCP->getPSO());
    cmdList->SetComputeRootSignature(surfaceVertexDensityCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { surfaceVertexDensityCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);

    // Transition surfaceVertexIndicesBuffer to SRV
    D3D12_RESOURCE_BARRIER surfaceVertexIndicesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertexIndicesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition surfaceVertDensityDispatch to an SRV
    D3D12_RESOURCE_BARRIER surfaceVertDensityDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    D3D12_RESOURCE_BARRIER barriers[2] = { surfaceVertexIndicesBufferBarrier, surfaceVertDensityDispatchBarrier };
    cmdList->ResourceBarrier(1, barriers);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, positionBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, cellsBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(2, surfaceVertexIndicesBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootShaderResourceView(3, surfaceVertDensityDispatch.getGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(4, surfaceVertDensityBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstants(5, 1, &gridConstants, 0);


    // Transition surfaceVertDensityDispatch to indirect argument buffer
    D3D12_RESOURCE_BARRIER surfaceVertDensityDispatchBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
    );

    cmdList->ResourceBarrier(1, &surfaceVertDensityDispatchBarrier2);

    // Dispatch
    cmdList->ExecuteIndirect(commandSignature, 1, surfaceVertDensityDispatch.getBuffer(), 0, nullptr, 0);

    context->executeCommandList(surfaceVertexDensityCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(surfaceVertexDensityCP->getCommandListID());
}

void FluidScene::computeSurfaceVertexNormal() {
    auto cmdList = surfaceVertexNormalCP->getCommandList();

    cmdList->SetPipelineState(surfaceVertexNormalCP->getPSO());
    cmdList->SetComputeRootSignature(surfaceVertexNormalCP->getRootSignature());

    // Set descriptor heap
    ID3D12DescriptorHeap* computeDescriptorHeaps[] = { surfaceVertexNormalCP->getDescriptorHeap()->Get() };
    cmdList->SetDescriptorHeaps(_countof(computeDescriptorHeaps), computeDescriptorHeaps);

    // Transition surfaceVertDensityBuffer to SRV
    D3D12_RESOURCE_BARRIER surfaceVertDensityBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    // Transition surfaceVertDensityDispatch to an SRV
    D3D12_RESOURCE_BARRIER surfaceVertDensityDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    D3D12_RESOURCE_BARRIER barriers[2] = { surfaceVertDensityBufferBarrier, surfaceVertDensityDispatchBarrier };
    cmdList->ResourceBarrier(1, barriers);

    // Set compute root descriptor table
    cmdList->SetComputeRootDescriptorTable(0, surfaceVertDensityBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, surfaceVertexIndicesBuffer.getSRVGPUDescriptorHandle());
    cmdList->SetComputeRootShaderResourceView(2, surfaceVertDensityDispatch.getGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(3, surfaceVertexNormalBuffer.getUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstants(4, 1, &gridConstants, 0);

    // Transition surfaceVertDensityDispatch to indirect argument buffer
    D3D12_RESOURCE_BARRIER surfaceVertDensityDispatchBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
    );

    cmdList->ResourceBarrier(1, &surfaceVertDensityDispatchBarrier2);

    // Dispatch
    cmdList->ExecuteIndirect(commandSignature, 1, surfaceVertDensityDispatch.getBuffer(), 0, nullptr, 0);

    context->executeCommandList(surfaceVertexNormalCP->getCommandListID());
    context->signalAndWaitForFence(fence, fenceValue);

    context->resetCommandList(surfaceVertexNormalCP->getCommandListID());
}

void FluidScene::releaseResources() {
    renderPipeline->releaseResources();
    bilevelUniformGridCP->releaseResources();
    surfaceBlockDetectionCP->releaseResources();
    surfaceCellDetectionCP->releaseResources();
    positionBuffer.releaseResources();
    cellsBuffer.releaseResources();
    blocksBuffer.releaseResources();
    surfaceBlockIndicesBuffer.releaseResources();
    surfaceBlockDispatch.releaseResources();
    surfaceHalfBlockDispatch.releaseResources();
    surfaceVerticesBuffer.releaseResources();
    surfaceVertexIndicesBuffer.releaseResources();
    surfaceVertDensityDispatch.releaseResources();
    surfaceVertDensityBuffer.releaseResources();
    surfaceVertexNormalBuffer.releaseResources();
}

void FluidScene::transitionBuffersToUAV(ID3D12GraphicsCommandList6* cmdList) {
    // Transition everything back to what it needs to be for the next frame
    // Cells buffer to UAV
    D3D12_RESOURCE_BARRIER cellsBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        cellsBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Blocks buffer to UAV
    D3D12_RESOURCE_BARRIER blocksBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        blocksBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    
    // Surface block indices buffer to UAV
    D3D12_RESOURCE_BARRIER surfaceBlockIndicesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockIndicesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Surface vertices buffer to UAV
    D3D12_RESOURCE_BARRIER surfaceVerticesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVerticesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Surface vertex indices buffer to UAV
    D3D12_RESOURCE_BARRIER surfaceVertexIndicesBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertexIndicesBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Surface vertex density buffer to UAV
    D3D12_RESOURCE_BARRIER surfaceVertDensityBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Surface vertex normal buffer to UAV
    D3D12_RESOURCE_BARRIER surfaceVertexNormalBufferBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertexNormalBuffer.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    // Dispatch buffers back to UAVs
    D3D12_RESOURCE_BARRIER surfaceBlockDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    D3D12_RESOURCE_BARRIER surfaceHalfBlockDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceHalfBlockDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    D3D12_RESOURCE_BARRIER surfaceVertDensityDispatchBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        surfaceVertDensityDispatch.getBuffer(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    D3D12_RESOURCE_BARRIER barriers[10] = { 
        cellsBufferBarrier,
        blocksBufferBarrier,
        surfaceBlockIndicesBufferBarrier,
        surfaceVerticesBufferBarrier,
        surfaceVertexIndicesBufferBarrier, 
        surfaceVertDensityBufferBarrier, 
        surfaceVertexNormalBufferBarrier2, 
        surfaceBlockDispatchBarrier, 
        surfaceHalfBlockDispatchBarrier, 
        surfaceVertDensityDispatchBarrier 
    };

    cmdList->ResourceBarrier(10, barriers);
}