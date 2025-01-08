#pragma once

#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"

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

private:
	ComPointer<ID3D12Resource> texture;
};
