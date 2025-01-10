#include "DescriptorHeap.h"

#include <cassert>
#include <stdexcept>

DescriptorHeap::DescriptorHeap(DXContext &context, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: descriptorCount(numberOfDescriptors)
{
	ComPointer<ID3D12Device6> device = context.getDevice();
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.NumDescriptors = numberOfDescriptors;
	description.Type = type;
	description.Flags = flags;

	if FAILED((device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&descriptorHeap)))) {
		throw std::runtime_error("Could not create descriptor heap");
	};
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

ComPointer<ID3D12DescriptorHeap>& DescriptorHeap::Get()
{
	return descriptorHeap;
}

ID3D12DescriptorHeap* DescriptorHeap::GetAddress()
{
	return descriptorHeap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandleAt(unsigned int index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandleAt(unsigned int index)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), index, descriptorSize);
}

unsigned int DescriptorHeap::GetNextAvailableIndex()
{
	if (currentDescriptorIndex >= descriptorCount)
	{
		assert(false && "Descriptor count within heap has been exceeded!");
		return 0;
	}

	unsigned int index = currentDescriptorIndex;
	currentDescriptorIndex++;
	return index;
}

unsigned int DescriptorHeap::GetDescriptorSize()
{
	return descriptorSize;
}

