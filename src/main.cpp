#include "main.h"

int main() {
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
    //ImGuiIO& io = initImGUI(context);

    //set mouse to use the window
    mouse->SetWindow(Window::get().getHWND());

    // Get the client area of the window
    RECT rect;
    GetClientRect(Window::get().getHWND(), &rect);
    float clientWidth = static_cast<float>(rect.right - rect.left);
    float clientHeight = static_cast<float>(rect.bottom - rect.top);

    //initialize scene
    Scene scene{camera.get(), &context};

    /*PBMPMConstants pbmpmCurrConstants = PBMPMConstants();
    PBMPMConstants pbmpmIterConstants = pbmpmCurrConstants;*/

    unsigned int renderOptions = 0;

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
            POINT mousePos;
            GetCursorPos(&mousePos);
            ScreenToClient(Window::get().getHWND(), &mousePos);
            float ndcX = (2.0f * mousePos.x / clientWidth) - 1.0f;
            float ndcY = 1.0f - (2.0f * mousePos.y / clientHeight);
        }

        auto objectWirePipeline = scene.getObjectWirePipeline();

        //begin frame
        Window::get().beginFrame(objectWirePipeline->getCommandList());

        //create viewport
        D3D12_VIEWPORT vp;
        Window::get().createViewport(vp, objectWirePipeline->getCommandList());

        //wire object render pass
        Window::get().setRT(objectWirePipeline->getCommandList());
        Window::get().setViewport(vp, objectWirePipeline->getCommandList());
        context.executeCommandList(objectWirePipeline->getCommandListID());

        // reset the first pipeline so it can end the frame
        context.resetCommandList(objectWirePipeline->getCommandListID());
        //end frame
        Window::get().endFrame(objectWirePipeline->getCommandList());
        // Execute command list
		context.executeCommandList(objectWirePipeline->getCommandListID());

        Window::get().present();
        context.resetCommandList(objectWirePipeline->getCommandListID());
    }

    // Scene should release all resources, including their pipelines
    scene.releaseResources();

    //flush pending buffer operations in swapchain
    context.flush(FRAME_COUNT);
    Window::get().shutdown();
}
