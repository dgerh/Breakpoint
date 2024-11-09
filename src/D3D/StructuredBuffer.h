#pragma once

#include <vector>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"
#include "D3D/Pipeline/Pipeline.h"
#include "DirectXMath.h"

using namespace DirectX;

class StructuredBuffer {
public:
	StructuredBuffer() = delete;
	StructuredBuffer(const void* data, unsigned int numEle, size_t eleSize);

	void passModelMatrixDataToGPU(DXContext& context, ComPointer<ID3D12DescriptorHeap> dh, ID3D12GraphicsCommandList5* cmdList);

	ComPointer<ID3D12Resource1>& getModelMatrixBuffer();

	void releaseResources();

private:
	ComPointer<ID3D12Resource1> modelMatrixBuffer;

	const void* data;
	unsigned int numElements;
	size_t elementSize;
};