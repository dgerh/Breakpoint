#include "ComputePipeline.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <comdef.h>
#include <iostream>

ComputePipeline::ComputePipeline(ID3D12Device* device, const std::string& computeShaderPath) : 
    computeShader(computeShaderPath)
{
    //LoadComputeShader(device);
}

ComputePipeline::~ComputePipeline() {
    ReleaseResources();
}

void ComputePipeline::CreateRootSignature(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParameters) {

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = static_cast<UINT>(rootParameters.size());
    rootSignatureDesc.pParameters = rootParameters.data();
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to serialize root signature.");
    }

    hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create root signature.");
    }
}

void ComputePipeline::LoadComputeShader(ID3D12Device* device) {
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.CS = { computeShader.getBuffer(), computeShader.getSize()};
    HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    if (FAILED(hr)) {
        _com_error err(hr);
        std::wcerr << L"Failed to create compute pipeline state: " << err.ErrorMessage() << std::endl;
        throw std::runtime_error("Failed to create compute pipeline state.");
    }
}

void ComputePipeline::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = descriptorCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvUavHeap));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create descriptor heap.");
    }
}

void ComputePipeline::SetConstantBuffer(ID3D12Device* device, UINT rootParameterIndex, size_t bufferSize, const void* data) {
    ComPtr<ID3D12Resource> constantBuffer;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = (bufferSize + 255) & ~255; // Align to 256 bytes
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer));

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create constant buffer.");
    }

    UINT8* mappedData;
    hr = constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to map constant buffer.");
    }
    memcpy(mappedData, data, bufferSize);

    constantBuffers.push_back(constantBuffer);
    constantBufferData.push_back(mappedData);
}

void ComputePipeline::SetSrvUavDescriptorTable(ID3D12GraphicsCommandList* cmdList, UINT rootParameterIndex) {
    cmdList->SetComputeRootDescriptorTable(rootParameterIndex, srvUavHeap->GetGPUDescriptorHandleForHeapStart());
}

void ComputePipeline::Dispatch(ID3D12GraphicsCommandList* cmdList, UINT x, UINT y, UINT z) {
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvUavHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetPipelineState(pipelineState.Get());
    cmdList->SetComputeRootSignature(rootSignature.Get());

    cmdList->Dispatch(x, y, z);
}

void ComputePipeline::ReleaseResources() {
    for (auto& cb : constantBuffers) {
        if (cb) {
            cb->Unmap(0, nullptr);
        }
    }
    constantBuffers.clear();
    constantBufferData.clear();
    srvUavHeap.Reset();
    pipelineState.Reset();
    rootSignature.Reset();
}