#include "ScreenQuad.h"

ScreenQuad::ScreenQuad(DXContext* context, RenderPipeline* pipeline)
	: Drawable(context, pipeline), texture(*context, 1920, 1080, "smiley.jpg"),
	samplerHeap(*context, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
{
	constructScene();
}

void ScreenQuad::constructScene() {
	renderPipeline->createPSOD();
	renderPipeline->createPipelineState(context->getDevice());

	// Create a quad that covers the entire screen
	std::vector<XMFLOAT3> positions = {
		{ -1.0f,  1.0f, 0.0f },
		{  1.0f,  1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f }
	};

	std::vector<UINT> indices = {
		0, 1, 2,
		2, 1, 3
	};

	std::vector<XMFLOAT2> uvs = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f }
	};

	vertexBuffer = VertexBuffer{ positions, (UINT)positions.size(), sizeof(XMFLOAT3) };
	indexBuffer = IndexBuffer{ indices, 6 };
	uvBuffer = StructuredBuffer{ uvs.data(), (UINT)uvs.size(), sizeof(XMFLOAT2) };

	auto cmdList = renderPipeline->getCommandList();
	vbv = vertexBuffer.passVertexDataToGPU(*context, cmdList);
	ibv = indexBuffer.passIndexDataToGPU(*context, cmdList);

	texture.uploadTextureSRV(*context, renderPipeline->getDescriptorHeap());

	// Create the sampler
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// Get the descriptor heap and create the sampler
	context->getDevice()->CreateSampler(&samplerDesc, samplerHeap.GetCPUHandleAt(0));
}

void ScreenQuad::draw() {
	// == IA ==
	auto cmdList = renderPipeline->getCommandList();
	cmdList->IASetVertexBuffers(0, 1, &vbv);
	cmdList->IASetIndexBuffer(&ibv);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// == PSO ==
	cmdList->SetPipelineState(renderPipeline->getPSO());
	cmdList->SetGraphicsRootSignature(renderPipeline->getRootSignature());

	// == ROOT ==
	ID3D12DescriptorHeap* descriptorHeaps[] = { renderPipeline->getDescriptorHeap()->GetAddress() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	cmdList->SetGraphicsRootDescriptorTable(1, renderPipeline->getDescriptorHeap()->GetGPUHandleAt(0)); // Descriptor table slot 1 for CBV
	cmdList->SetGraphicsRootDescriptorTable(2, texture.getTextureGPUHandle());
	cmdList->SetGraphicsRootDescriptorTable(3, samplerHeap.GetGPUHandleAt(0)); // Sampler

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}