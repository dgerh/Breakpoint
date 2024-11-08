#include <iostream>

#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "Support/Window.h"
#include "Support/Shader.h"

#include "Debug/DebugLayer.h"

#include "D3D/DXContext.h"
#include "D3D/RenderPipelineHelper.h"
#include "D3D/RenderPipeline.h"
#include "D3D/VertexBuffer.h"
#include "D3D/IndexBuffer.h"
#include "D3D/ModelMatrixBuffer.h"
#include "D3D/ComputePipeline.h"

#include "Scene/Camera.h"

struct ParticleParams {
    float gravity;
    float deltaTime;
    float padding[2]; // Padding to align to 16 bytes
};
