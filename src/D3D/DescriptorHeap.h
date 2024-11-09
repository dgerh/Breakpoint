#pragma once
#include <d3d12.h>
#include "./includes/d3dx12.h"
#include <wrl.h>
#include "DXContext.h"

// Sourced from https://github.com/stefanpgd/Compute-DirectX12-Tutorial/

class DescriptorHeap
{
public:
	DescriptorHeap(DXContext &context, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	ComPointer<ID3D12DescriptorHeap>& Get();
	ID3D12DescriptorHeap* GetAddress();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAt(unsigned int index);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAt(unsigned int index);

	unsigned int GetNextAvailableIndex();
	unsigned int GetDescriptorSize();

	void releaseResources() { descriptorHeap.Release(); }

private:
	ComPointer<ID3D12DescriptorHeap> descriptorHeap;

	unsigned int descriptorSize;
	unsigned int descriptorCount;
	unsigned int currentDescriptorIndex = 0;
};