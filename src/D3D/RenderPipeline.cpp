#include "RenderPipeline.h"

RenderPipeline::RenderPipeline(std::string vertexShaderName, std::string fragShaderName, std::string rootSignatureShaderName, DXContext& context) 
	: vertexShader(vertexShaderName),
	  fragShader(fragShaderName),
	  rootSignatureShader(rootSignatureShaderName)
{
	context.getDevice()->CreateRootSignature(0, rootSignatureShader.getBuffer(), rootSignatureShader.getSize(), IID_PPV_ARGS(&rootSignature));

	// Describe and create an SRV heap with a single descriptor for our model matrix buffer.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;                  // We only need one descriptor for now
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // Heap type for CBVs, SRVs, and UAVs
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Make it visible to shaders

	if (FAILED(context.getDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)))) {
		throw std::runtime_error("Could not create descriptor heap for SRV");
	}
}

ComPointer<ID3D12RootSignature> RenderPipeline::getRootSignature()
{
	return this->rootSignature;
}

ComPointer<ID3D12DescriptorHeap> RenderPipeline::getSrvHeap()
{
	return this->srvHeap;
}

void RenderPipeline::releaseResources()
{
	this->rootSignature.Release();
	this->srvHeap.Release();
}