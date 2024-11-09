#include "DXContext.h"

DXContext::DXContext() {

    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)))) {
        //handle could not create dxgi factory
        throw std::runtime_error("Could not create dxgi factory");
    }

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        //handle could not create device
        throw std::runtime_error("Could not create device");
    }

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    cmdQueueDesc.NodeMask = 0;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue)))) {
        //handle could not create command queue
        throw std::runtime_error("Could not create command queue");
    }

    if (FAILED(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        //handle could not create fence
        throw std::runtime_error("Could not create fence");
    }

    fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    if (!fenceEvent) {
        //handle could not create fence event
        throw std::runtime_error("Could not create fence event");
    }

    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)))) {
        //handle cannot create cmd allocator
        throw std::runtime_error("Could not create command allocator");
    }

    if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&cmdList)))) {
        //handle could not create cmd list
        throw std::runtime_error("Could not create command list");
    }
}

DXContext::~DXContext() {
    cmdList.Release();
    cmdAllocator.Release();
    if (fenceEvent)
    {
        CloseHandle(fenceEvent);
    }
    fence.Release();
    cmdQueue.Release();
    device.Release();
    dxgiFactory.Release();
}

void DXContext::signalAndWait() {
    cmdQueue->Signal(fence, ++fenceValue);
    if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent))) {
        if (WaitForSingleObject(fenceEvent, 20000) != WAIT_OBJECT_0) {
            std::exit(-1);
        }
    } else {
        std::exit(-1);
    }
}

ID3D12GraphicsCommandList6* DXContext::initCommandList()
{
    cmdAllocator->Reset();
    cmdList->Reset(cmdAllocator, nullptr);
    return cmdList;
}

void DXContext::executeCommandList() {
    if (SUCCEEDED(cmdList->Close())) {
        ID3D12CommandList* lists[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, lists);
        signalAndWait();
    }
}

void DXContext::flush(size_t count) {
    for (size_t i = 0; i < count; i++) {
        signalAndWait();
    }
}

ComPointer<IDXGIFactory7>& DXContext::getFactory() {
    return dxgiFactory;
}

ComPointer<ID3D12Device6>& DXContext::getDevice() {
    return device;
}

ComPointer<ID3D12CommandQueue>& DXContext::getCommandQueue() {
    return cmdQueue;
}
