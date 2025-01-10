#pragma once

#include <vector>

#include "Drawable.h"
#include "Mesh.h"
#include "../D3D/Texture.h"

#include "DirectXMath.h"

class ScreenQuad : public Drawable {
public:
	ScreenQuad(DXContext* context, RenderPipeline* pipeline);

	void constructScene();

	void draw();

	size_t getSceneSize();

	void releaseResources();

private:
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	StructuredBuffer uvBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	Texture texture;
};