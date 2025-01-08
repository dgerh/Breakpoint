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

    // Upload the resources to the GPU.
    auto uploadResourcesFinished = upload.End(context.getCommandQueue());

    // Wait for the upload thread to terminate
    uploadResourcesFinished.wait();
}