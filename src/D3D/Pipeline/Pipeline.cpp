#include "Pipeline.h"

Pipeline::Pipeline(std::string rootSignatureShaderName, DXContext& context, CommandListID cmdID,
	D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: rootSignatureShader(rootSignatureShaderName), descriptorHeap(context, type, numberOfDescriptors, flags), cmdID(cmdID),
	cmdList(context.createCommandList(cmdID))
{
	context.resetCommandList(cmdID);
	context.getDevice()->CreateRootSignature(0, rootSignatureShader.getBuffer(), rootSignatureShader.getSize(), IID_PPV_ARGS(&rootSignature));
}

ComPointer<ID3D12RootSignature>& Pipeline::getRootSignature()
{
	return this->rootSignature;
}

DescriptorHeap* Pipeline::getDescriptorHeap()
{
	return &descriptorHeap;
}

void Pipeline::releaseResources()
{
	rootSignature.Release();
	descriptorHeap.releaseResources();
	pso.Release();
}