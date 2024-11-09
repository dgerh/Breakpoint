#pragma once

#include "../../Support/WinInclude.h"
#include "../../Support/Shader.h"
#include "../../Support/ComPointer.h"
#include "../../Support/Window.h"
#include "../DescriptorHeap.h"

class Pipeline {
public:
	Pipeline() = delete;
	Pipeline(std::string rootSignatureShaderName, DXContext& context,
		D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	~Pipeline() = default;

	virtual void createPSOD() = 0;
	virtual void createPipelineState(ComPointer<ID3D12Device6> device) = 0;

	ComPointer<ID3D12RootSignature>& getRootSignature();
	ComPointer<ID3D12DescriptorHeap>& getDescriptorHeap();
	ComPointer<ID3D12PipelineState>& getPSO() { return pso; }

	void releaseResources();

protected:
	Shader rootSignatureShader;

	ComPointer<ID3D12RootSignature> rootSignature;
	DescriptorHeap descriptorHeap;
	ComPointer<ID3D12PipelineState> pso;
};