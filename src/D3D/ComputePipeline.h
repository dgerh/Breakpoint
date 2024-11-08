#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include "Support/Shader.h"

using Microsoft::WRL::ComPtr;

class ComputePipeline {
public:
    ComputePipeline(ID3D12Device* device, const std::string& computeShaderPath);
    ~ComputePipeline();

    // Initialize root signature with custom root parameters
    void CreateRootSignature(ID3D12Device* device, const std::vector<D3D12_ROOT_PARAMETER>& rootParameters);

    // Load the compute shader from file
    void LoadComputeShader(ID3D12Device* device);

    // Create a descriptor heap for SRV/UAV resources
    void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount);

    // Create and update a constant buffer with custom data
    void SetConstantBuffer(ID3D12Device* device, UINT rootParameterIndex, size_t bufferSize, const void* data);

    // Set SRV/UAV descriptor table in the root signature
    void SetSrvUavDescriptorTable(ID3D12GraphicsCommandList* cmdList, UINT rootParameterIndex);

    // Dispatch the compute shader
    void Dispatch(ID3D12GraphicsCommandList* cmdList, UINT x, UINT y, UINT z);

    // Release resources
    void ReleaseResources();

private:
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12DescriptorHeap> srvUavHeap;

    std::vector<ComPtr<ID3D12Resource>> constantBuffers; // Store all constant buffers
    std::vector<UINT8*> constantBufferData; // Mapped CPU-accessible data pointers

	Shader computeShader;
};
