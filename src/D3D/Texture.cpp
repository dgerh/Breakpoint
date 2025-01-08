#include "Texture.h"

Texture::Texture(DXContext& context, UINT width, UINT height) {
    D3D12_RESOURCE_DESC txtDesc = {};
    txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    context.getDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &txtDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture));

    ResourceUploadBatch upload = ResourceUploadBatch(context.getDevice());
	upload.Begin();

    HRESULT hr = DirectX::CreateWICTextureFromFile(
        context.getDevice(),
        upload,
        L"../Resources/Textures/DefaultTexture.png",
        &texture);

    // Upload the resources to the GPU.
    auto uploadResourcesFinished = upload.End(context.getCommandQueue());

    // Wait for the upload thread to terminate
    uploadResourcesFinished.wait();
}