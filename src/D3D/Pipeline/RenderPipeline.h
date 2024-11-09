#pragma once
#include "Pipeline.h"

class RenderPipeline : public Pipeline {
public:
	RenderPipeline() = delete;
	RenderPipeline(std::string vertexShaderName, std::string fragShaderName, std::string rootSignatureShaderName, DXContext& context,
		D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	Shader& getVertexShader() { return vertexShader; }
	Shader& getFragmentShader() { return fragShader; }

	void createPSOD() override;

	void createPipelineState(ComPointer<ID3D12Device6> device) override;

private:
	Shader vertexShader, fragShader;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};

};
