#include "main.h"

int main() {
    //initialize for texture loading
    /*HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cout << "could not initialize for texture loading\n";
        Window::get().shutdown();
        return false;
    }*/

    //set up DX, window, keyboard mouse
    DebugLayer debugLayer = DebugLayer();
    DXContext context = DXContext();
    std::unique_ptr<Camera> camera = std::make_unique<Camera>();
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

    if (!Window::get().init(&context, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        //handle could not initialize window
        std::cout << "could not initialize window\n";
        Window::get().shutdown();
        return false;
    }

    //initialize ImGUI
    ImGuiIO& io = initImGUI(context);

    //set mouse to use the window
    mouse->SetWindow(Window::get().getHWND());

    // Get the client area of the window
    RECT rect;
    GetClientRect(Window::get().getHWND(), &rect);
    float clientWidth = static_cast<float>(rect.right - rect.left);
    float clientHeight = static_cast<float>(rect.bottom - rect.top);

    //initialize scene
    Scene scene{camera.get(), &context};

    PBMPMConstants pbmpmCurrConstants = scene.getPBMPMConstants();
    PBMPMConstants pbmpmIterConstants = pbmpmCurrConstants;

    unsigned int renderOptions = 0;

    Texture myDemoTexture(context, 1920, 1080, "smiley.jpg" );

    while (!Window::get().getShouldClose()) {
        //update window
        Window::get().update();
        if (Window::get().getShouldResize()) {
            //flush pending buffer operations in swapchain
            context.flush(FRAME_COUNT);
            Window::get().resize();
            camera->updateAspect((float)Window::get().getWidth() / (float)Window::get().getHeight());
        }

        auto kState = keyboard->GetState();
        auto mState = mouse->GetState();
        mouse->SetMode(mState.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
        camera->kmStateCheck(kState, mState);

        if (mState.rightButton) {

			// If right mouse button is pressed, we should update constants

            if (kState.LeftShift) {
                // Pulling Fluid
                pbmpmIterConstants.mouseFunction = 2;
            }
            else if (kState.LeftAlt) {
                // Grab Fluid Ball
                pbmpmIterConstants.mouseFunction = 1;
            }
            else {
                // Pushing Fluid
                pbmpmIterConstants.mouseFunction = 0;
            }

            // enable mouse force
            pbmpmIterConstants.mouseActivation = 1;

            POINT mousePos;
            GetCursorPos(&mousePos);
            ScreenToClient(Window::get().getHWND(), &mousePos);
            float ndcX = (2.0f * mousePos.x / clientWidth) - 1.0f;
            float ndcY = 1.0f - (2.0f * mousePos.y / clientHeight);

            XMFLOAT4 prevMousePos = pbmpmIterConstants.mousePosition;
            ComputeMouseRay(
                Window::get().getHWND(),
                ndcX,
                ndcY,
                camera->getProjMat(),
                camera->getViewMat(),
                pbmpmIterConstants.mousePosition,
                pbmpmIterConstants.mouseRayDirection
            );
        }
        else {
            pbmpmIterConstants.mouseActivation = 0;
        }

        //compute pbmpm + mesh shader
        //scene.compute(renderModeType != 2);

        //get pipelines
        auto renderPipeline = scene.getPBMPMRenderPipeline();
        auto fluidMeshPipeline = scene.getFluidMeshPipeline();
		auto elasticMeshPipeline = scene.getElasticMeshPipeline();
		auto viscoMeshPipeline = scene.getViscoMeshPipeline();
		auto sandMeshPipeline = scene.getSandMeshPipeline();
		//auto snowMeshPipeline = scene.getSnowMeshPipeline();
        auto objectWirePipeline = scene.getObjectWirePipeline();
        auto objectSolidPipeline = scene.getObjectSolidPipeline();
		auto screenQuadPipeline = scene.getScreenQuadPipeline();
        //whichever pipeline renders first should begin and end the frame
        //auto firstPipeline = objectWirePipeline;
		auto firstPipeline = screenQuadPipeline;

        //begin frame
        Window::get().beginFrame(firstPipeline->getCommandList());

        //create viewport
        D3D12_VIEWPORT vp;
        Window::get().createViewport(vp, firstPipeline->getCommandList());

        //wire object render pass
        /*Window::get().setRT(objectWirePipeline->getCommandList());
        Window::get().setViewport(vp, objectWirePipeline->getCommandList());
        if (renderGrid) scene.drawGrid();
        if (renderSpawn) scene.drawSpawners();
        context.executeCommandList(objectWirePipeline->getCommandListID());*/

        //solid object render pass
        /*Window::get().setRT(objectSolidPipeline->getCommandList());
        Window::get().setViewport(vp, objectSolidPipeline->getCommandList());
        scene.drawSolidObjects();
        context.executeCommandList(objectSolidPipeline->getCommandListID());*/
        
        //full screen render pass
        Window::get().setRT(screenQuadPipeline->getCommandList());
        Window::get().setViewport(vp, screenQuadPipeline->getCommandList());
        scene.drawScreenQuad();
        context.executeCommandList(screenQuadPipeline->getCommandListID());

        //particles + imgui render pass
        /*Window::get().setRT(renderPipeline->getCommandList());
        Window::get().setViewport(vp, renderPipeline->getCommandList());*/
		// Only draw particles if we are not in the mesh shading mode
        /*if (renderModeType != 0) {
            scene.drawPBMPM();
        }*/

        //fluid mesh render pass
        /*if (scene.renderToggles[0]) {
            Window::get().setRT(fluidMeshPipeline->getCommandList());
            Window::get().setViewport(vp, fluidMeshPipeline->getCommandList());
            if (renderModeType != 2) scene.drawFluid(meshletRenderType, toonShadingLevels);
            context.executeCommandList(fluidMeshPipeline->getCommandListID());
        }*/

        // elastic mesh render pass
        /*if (scene.renderToggles[1]) {
            Window::get().setRT(elasticMeshPipeline->getCommandList());
            Window::get().setViewport(vp, elasticMeshPipeline->getCommandList());
            if (renderModeType != 2) scene.drawElastic(meshletRenderType, toonShadingLevels);
            context.executeCommandList(elasticMeshPipeline->getCommandListID());
        }*/

        // sand mesh render pass
        /*if (scene.renderToggles[2]) {
            Window::get().setRT(sandMeshPipeline->getCommandList());
            Window::get().setViewport(vp, sandMeshPipeline->getCommandList());
            if (renderModeType != 2) scene.drawSand(meshletRenderType, toonShadingLevels);
            context.executeCommandList(sandMeshPipeline->getCommandListID());
        }*/

		// visco mesh render pass
        /*if (scene.renderToggles[3]) {
			Window::get().setRT(viscoMeshPipeline->getCommandList());
			Window::get().setViewport(vp, viscoMeshPipeline->getCommandList());
			if (renderModeType != 2) scene.drawVisco(meshletRenderType, toonShadingLevels);
			context.executeCommandList(viscoMeshPipeline->getCommandListID());
		}*/

		// snow mesh render pass
		/*if (scene.renderToggles[4]) {
			Window::get().setRT(snowMeshPipeline->getCommandList());
			Window::get().setViewport(vp, snowMeshPipeline->getCommandList());
			if (renderModeType != 2) scene.drawSnow(meshletRenderType, toonShadingLevels);
			context.executeCommandList(snowMeshPipeline->getCommandListID());
		}*/

        //set up ImGUI for frame
  //      ImGui_ImplDX12_NewFrame();
  //      ImGui_ImplWin32_NewFrame();
  //      ImGui::NewFrame();

  //      //draw ImGUI
		//drawImGUIWindow(pbmpmIterConstants, io,
  //          scene.getFluidIsovalue(), 
  //          scene.getFluidKernelScale(), 
  //          scene.getFluidKernelRadius(),
		//	scene.getElasticIsovalue(),
		//	scene.getElasticKernelScale(),
		//	scene.getElasticKernelRadius(),
		//	scene.getSandIsovalue(),
		//	scene.getSandKernelScale(),
		//	scene.getSandKernelRadius(),
  //          scene.getViscoIsovalue(),
  //          scene.getViscoKernelScale(),
  //          scene.getViscoKernelRadius(),
  //          scene.getPBMPMSubstepCount(),
  //          scene.getNumParticles());

  //      //render ImGUI
  //      ImGui::Render();
  //      if (pbmpmIterConstants.mouseActivation == 1 || !PBMPMScene::constantsEqual(pbmpmIterConstants, pbmpmCurrConstants)) {
  //          scene.updatePBMPMConstants(pbmpmIterConstants);
  //          pbmpmCurrConstants = pbmpmIterConstants;
  //      }

  //      renderPipeline->getCommandList()->SetDescriptorHeaps(1, &imguiSRVHeap);
  //      ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderPipeline->getCommandList());

        /*context.executeCommandList(renderPipeline->getCommandListID());*/

        // reset the first pipeline so it can end the frame
        context.resetCommandList(firstPipeline->getCommandListID());
        //end frame
        Window::get().endFrame(firstPipeline->getCommandList());
        // Execute command list
		context.executeCommandList(firstPipeline->getCommandListID());

        Window::get().present();
		/*context.resetCommandList(renderPipeline->getCommandListID());
		if (scene.renderToggles[0]) {
			context.resetCommandList(fluidMeshPipeline->getCommandListID());
		}
        if (scene.renderToggles[1]) {
            context.resetCommandList(elasticMeshPipeline->getCommandListID());
        }
        if (scene.renderToggles[2]) {
			context.resetCommandList(sandMeshPipeline->getCommandListID());
		}
		if (scene.renderToggles[3]) {
			context.resetCommandList(viscoMeshPipeline->getCommandListID());
		}*/
		/*if (scene.renderToggles[4]) {
			context.resetCommandList(snowMeshPipeline->getCommandListID());
		}*/
        /*context.resetCommandList(objectWirePipeline->getCommandListID());
        context.resetCommandList(objectSolidPipeline->getCommandListID());*/
		context.resetCommandList(screenQuadPipeline->getCommandListID());
    }

    // Scene should release all resources, including their pipelines
    scene.releaseResources();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    imguiSRVHeap->Release();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);
    Window::get().shutdown();
}
