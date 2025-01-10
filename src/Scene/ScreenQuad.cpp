#include "ScreenQuad.h"

ScreenQuad::ScreenQuad(DXContext* context, RenderPipeline* pipeline)
	: Drawable(context, pipeline), texture(*context, 1920, 1080, "smiley.jpg")
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
	indexBuffer = IndexBuffer{ indices, (UINT)indices.size() };
	uvBuffer = StructuredBuffer{ uvs.data(), (UINT)uvs.size(), sizeof(XMFLOAT2) };

	auto cmdList = renderPipeline->getCommandList();
	vbv = vertexBuffer.passVertexDataToGPU(*context, cmdList);
	ibv = indexBuffer.passIndexDataToGPU(*context, cmdList);

	//texture = Texture(*context, 1920, 1080, "smiley.jpg");
	texture.uploadTextureSRV(*context, renderPipeline->getDescriptorHeap());

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

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}