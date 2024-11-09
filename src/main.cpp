#include "main.h"
#include "Scene/Geometry.h"

// This should probably go somewhere else
void createDefaultViewport(D3D12_VIEWPORT& vp, ID3D12GraphicsCommandList5* cmdList) {
    vp.TopLeftX = vp.TopLeftY = 0;
    vp.Width = Window::get().getWidth();
    vp.Height = Window::get().getHeight();
    vp.MinDepth = 1.f;
    vp.MaxDepth = 0.f;
    cmdList->RSSetViewports(1, &vp);
    RECT scRect;
    scRect.left = scRect.top = 0;
    scRect.right = Window::get().getWidth();
    scRect.bottom = Window::get().getHeight();
    cmdList->RSSetScissorRects(1, &scRect);
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

    //pass triangle data to gpu, get vertex buffer view
    int instanceCount = 8;

    // Create Test Model Matrices
    std::vector<XMFLOAT4X4> modelMatrices;
    // Populate modelMatrices with transformation matrices for each instance
    for (int i = 0; i < instanceCount; ++i) {
        XMFLOAT4X4 model;
        XMStoreFloat4x4(&model, XMMatrixTranslation(i * 0.2f, i * 0.2f, i * 0.2f)); // Example transformation
        modelMatrices.push_back(model);
    }

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

    RenderPipeline basicPipeline("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", context,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	/*MeshPipeline basicPipeline("MeshShader.cso", "PixelShader.cso", "RootSignature.cso", context,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);*/

    // === Pipeline state ===
    basicPipeline.createPSOD();

    //output merger
    basicPipeline.createPipelineState(context.getDevice());

    StructuredBuffer modelBuffer = StructuredBuffer(modelMatrices.data(), instanceCount, sizeof(XMFLOAT4X4));
	modelBuffer.passModelMatrixDataToGPU(context, basicPipeline.getDescriptorHeap(), cmdList);

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

        if (mState.positionMode == Mouse::MODE_RELATIVE) {
            camera->rotateOnX(-mState.y * 0.01);
            camera->rotateOnY(mState.x * 0.01);
            camera->rotate();
        }

        mouse->SetMode(mState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

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
        cmdList->SetPipelineState(basicPipeline.getPSO());
        cmdList->SetGraphicsRootSignature(basicPipeline.getRootSignature());
        // == ROOT ==

        ID3D12DescriptorHeap* descriptorHeaps[] = { basicPipeline.getDescriptorHeap().Get() };
        cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        cmdList->SetGraphicsRootDescriptorTable(1, basicPipeline.getDescriptorHeap()->GetGPUDescriptorHandleForHeapStart()); // Descriptor table slot 1 for SRV

        auto viewMat = camera->getViewMat();
        auto projMat = camera->getProjMat();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);

        // Draw
        cmdList->DrawIndexedInstanced(circleData.second.size(), instanceCount, 0, 0, 0);
		//cmdList->DispatchMesh(1, 1, 1);

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