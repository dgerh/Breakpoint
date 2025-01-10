#pragma once

#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"
#include "D3D/DescriptorHeap.h"

#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>

#include <iostream>
#include <filesystem>

using namespace DirectX;

class Texture {
public:
	Texture() = delete;
	Texture(DXContext& context, UINT width, UINT height, std::string texName);
	~Texture() = default; //make sure to release resources later

	void uploadTextureSRV(DXContext& context, DescriptorHeap* dh);

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE getTextureCPUHandle() { return textureCPUHandle; }
	inline CD3DX12_GPU_DESCRIPTOR_HANDLE getTextureGPUHandle() { return textureGPUHandle; }

private:
	ComPointer<ID3D12Resource> texture;

	CD3DX12_CPU_DESCRIPTOR_HANDLE textureCPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureGPUHandle;
};
