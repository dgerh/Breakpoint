#include "main.h"
#include "Scene/Geometry.h"
#include "Support/directx/d3dx12.h"

void setupGravityComputePipeline(ComputePipeline &pipeline, DXContext &context, unsigned int instanceCount, std::vector<XMFLOAT3> &initialPositions) {
    // Define root parameters for the compute shader
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;

    // Descriptor table for SRV (input) and UAV (output)
    D3D12_DESCRIPTOR_RANGE srvUavRange[2];
    srvUavRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvUavRange[0].NumDescriptors = 1;
    srvUavRange[0].BaseShaderRegister = 0;

    srvUavRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    srvUavRange[1].NumDescriptors = 1;
    srvUavRange[1].BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER srvUavTable = {};
    srvUavTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    srvUavTable.DescriptorTable.NumDescriptorRanges = _countof(srvUavRange);
    srvUavTable.DescriptorTable.pDescriptorRanges = srvUavRange;
    srvUavTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters.push_back(srvUavTable);

    // CBV for ParticleParams
    D3D12_ROOT_PARAMETER cbvParam = {};
    cbvParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    cbvParam.Descriptor.ShaderRegister = 0;
    cbvParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters.push_back(cbvParam);

    // Create root signature and descriptor heap
    pipeline.CreateRootSignature(context.getDevice(), rootParameters);
    pipeline.CreateDescriptorHeap(context.getDevice(), 2); // 1 SRV, 1 UAV
    pipeline.LoadComputeShader(context.getDevice());

    // Create input position buffer (SRV) on GPU

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> positionBuffer;
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceCount * sizeof(XMFLOAT3));
    context.getDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&positionBuffer)
    );

    // Map and copy initial positions
    CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_DEFAULT);
    UINT8* positionData;
    positionBuffer->Map(0, nullptr, reinterpret_cast<void**>(&positionData));
    memcpy(positionData, initialPositions.data(), initialPositions.size() * sizeof(XMFLOAT3));
    positionBuffer->Unmap(0, nullptr);

    // Create output model matrix buffer (UAV) on GPU
    ComPtr<ID3D12Resource> modelMatrixBuffer;
    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceCount * sizeof(XMFLOAT4X4));
    context.getDevice()->CreateCommittedResource(
        &uploadProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&modelMatrixBuffer)
    );
}

int main() {
    DebugLayer debugLayer = DebugLayer();
    DXContext context = DXContext();
    auto* cmdList = context.initCommandList();
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

    if (!Window::get().init(&context, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        //handle could not initialize window
        std::cout << "could not initialize window\n";
        Window::get().shutdown();
        return false;
    }

    mouse->SetWindow(Window::get().getHWND());

    // Define gravity and delta time constants
    float gravity = 9.8f;
    float deltaTime = 0.016f; // assuming ~60 FPS

    //pass triangle data to gpu, get vertex buffer view
    unsigned int instanceCount = 8;

    // Create Test Model Matrices
    std::vector<XMFLOAT4X4> modelMatrices;
    //// Populate modelMatrices with transformation matrices for each instance
    for (int i = 0; i < instanceCount; ++i) {
        XMFLOAT4X4 model;
        XMStoreFloat4x4(&model, XMMatrixTranslation(i * 0.2f, i * 0.2f, i * 0.2f)); // Example transformation
        modelMatrices.push_back(model);
    }

    // Input buffer: initial positions for each instance
    std::vector<XMFLOAT3> initialPositions(instanceCount);
    for (int i = 0; i < instanceCount; ++i) {
        initialPositions[i] = XMFLOAT3(i * 0.2f, 0.0f, 0.0f); // Example initial positions
    }

	// Setup compute pipeline
    ComputePipeline computePipeline(context.getDevice(), "ParticleComputeShader.cso");
	setupGravityComputePipeline(computePipeline, context, instanceCount, initialPositions);

	// Create circle geometry
	auto circleData = generateCircle(0.05f, 32);
   
    VertexBuffer vertBuffer = VertexBuffer(circleData.first, circleData.first.size() * sizeof(XMFLOAT3), sizeof(XMFLOAT3));
    auto vbv = vertBuffer.passVertexDataToGPU(context, cmdList);

    IndexBuffer idxBuffer = IndexBuffer(circleData.second, circleData.second.size() * sizeof(unsigned int));
    auto ibv = idxBuffer.passIndexDataToGPU(context, cmdList);
    
    //Transition both buffers to their usable states
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    // Vertex buffer barrier
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = vertBuffer.getVertexBuffer().Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Index buffer barrier
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = idxBuffer.getIndexBuffer().Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdList->ResourceBarrier(2, barriers);
    RenderPipeline basicPipeline("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", context);

    // === Create root signature ===
    ComPointer<ID3D12RootSignature> rootSignature = basicPipeline.getRootSignature();

    // === Pipeline state ===
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};
    createShaderPSOD(gfxPsod, rootSignature, basicPipeline.getVertexShader(), basicPipeline.getFragmentShader());
    
    //output merger
    ComPointer<ID3D12PipelineState> pso;
    context.getDevice()->CreateGraphicsPipelineState(&gfxPsod, IID_PPV_ARGS(&pso));

    ModelMatrixBuffer modelBuffer = ModelMatrixBuffer(modelMatrices, instanceCount);
	modelBuffer.passModelMatrixDataToGPU(context, basicPipeline, cmdList);

    while (!Window::get().getShouldClose()) {
        //update window
        Window::get().update();
        if (Window::get().getShouldResize()) {
            //flush pending buffer operations in swapchain
            context.flush(FRAME_COUNT);
            Window::get().resize();
            camera->updateAspect((float)Window::get().getWidth() / (float)Window::get().getHeight());
        }
        
        //check keyboard state
        auto kState = keyboard->GetState();
        if (kState.W) {
            camera->translate({ 0, 0, 0.0005 });
        }
        if (kState.A) {
            camera->translate({ -0.0005, 0, 0 });
        }
        if (kState.S) {
            camera->translate({ 0, 0, -0.0005 });
        }
        if (kState.D) {
            camera->translate({ 0.0005, 0, 0 });
        }
        if (kState.Space) {
            camera->translate({ 0, 0.0005, 0 });
        }
        if (kState.LeftControl) {
            camera->translate({ 0, -0.0005, 0 });
        }

        //check mouse state
        auto mState = mouse->GetState();

        //update camera
        camera->updateViewMat();

        //begin draw
        cmdList = context.initCommandList();

        //draw to window
        Window::get().beginFrame(cmdList);

        //draw
        // == IA ==
        cmdList->IASetVertexBuffers(0, 1, &vbv);
		cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // == RS ==
        D3D12_VIEWPORT vp;
        createDefaultViewport(vp, cmdList);
        // == PSO ==
        cmdList->SetPipelineState(pso);
        cmdList->SetGraphicsRootSignature(rootSignature);
        // == ROOT ==

        ID3D12DescriptorHeap* descriptorHeaps[] = { basicPipeline.getSrvHeap().Get() };
        cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        cmdList->SetGraphicsRootDescriptorTable(1, basicPipeline.getSrvHeap()->GetGPUDescriptorHandleForHeapStart()); // Descriptor table slot 1 for SRV

        auto viewMat = camera->getViewMat();
        auto projMat = camera->getProjMat();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);

        // Draw
        cmdList->DrawIndexedInstanced(circleData.second.size(), instanceCount, 0, 0, 0);

        Window::get().endFrame(cmdList);

        //finish draw, present
        context.executeCommandList();
        Window::get().present();
    }

    // Close
    vertBuffer.releaseResources();
    idxBuffer.releaseResources();
	modelBuffer.releaseResources();
	basicPipeline.releaseResources();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);
    Window::get().shutdown();
}