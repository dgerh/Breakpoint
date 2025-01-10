#include "Texture.h"

Texture::Texture(DXContext& context, UINT width, UINT height, std::string texName) {

    std::filesystem::path texturePath = std::filesystem::current_path() / "textures" / texName;

    ResourceUploadBatch upload = ResourceUploadBatch(context.getDevice());
	upload.Begin();

    HRESULT hr = DirectX::CreateWICTextureFromFile(
        context.getDevice(),
        upload,
        texturePath.c_str(),
        &texture);

	if (FAILED(hr)) {
		std::cout << "could not load texture\n";
	}

    upload.End(context.getCommandQueue()).wait();
}

void Texture::uploadTextureSRV(DXContext& context, DescriptorHeap* dh) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;

    auto nextIdx = dh->GetNextAvailableIndex();
    context.getDevice()->CreateShaderResourceView(texture.Get(), &srvDesc, dh->GetCPUHandleAt(nextIdx));
    textureCPUHandle = dh->GetCPUHandleAt(nextIdx);
    textureGPUHandle = dh->GetGPUHandleAt(nextIdx);
}