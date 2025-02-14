#pragma once
#include <vector>
#include "Drawable.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"
#include "../D3D/StructuredBuffer.h"
#include "../Shaders/constants.h"

struct GridConstants {
    int numParticles;
    XMINT3 gridDim;
    XMFLOAT3 minBounds;
    float resolution;
    float kernelScale;
    float kernelRadius;
    int material;
};

// TODO: can just combine this with grid constants
struct MeshShadingConstants {
    XMMATRIX viewProj;
    XMINT3 dimensions;
    float resolution;
    XMFLOAT3 minBounds;
    unsigned int renderMeshlets;
    XMFLOAT3 cameraPos;
    unsigned int renderOptions;
    XMFLOAT3 cameraLook;
    float isovalue;
};

struct Cell {
    int particleCount;
    int particleIndices[MAX_PARTICLES_PER_CELL];
};

struct Block {
    int nonEmptyCellCount;
};

class MeshShadingScene : public Drawable {
public:
    MeshShadingScene() = delete;
    MeshShadingScene(DXContext* context, 
               RenderPipeline *pipeline, 
               ComputePipeline* bilevelUniformGridCP, 
               ComputePipeline* surfaceBlockDetectionCP,
               ComputePipeline* surfaceCellDetectionCP,
               ComputePipeline* surfaceVertexCompactionCP,
               ComputePipeline* surfaceVertexDensityCP,
               ComputePipeline* surfaceVertexNormalCP,
               ComputePipeline* bufferClearCP,
               ComputePipeline* dispatchArgDivideCP,
               MeshPipeline* fluidMeshPipeline,
		int material, float isovalue, float kernelScale, float kernelRadius);

    void compute(
        StructuredBuffer* positionsBuffer,
        int numParticles
    );
    void draw(Camera* camera, unsigned int renderMeshlets, unsigned int renderOptions);
    void constructScene();
    void computeBilevelUniformGrid();
    void computeSurfaceBlockDetection();
    void computeSurfaceCellDetection();
    void compactSurfaceVertices();
    void computeSurfaceVertexDensity();
    void computeSurfaceVertexNormal();
    void releaseResources();

    float* getIsovalue() { return &isovalue; }
    float* getKernelScale() { return &kernelScale; }
    float* getKernelRadius() { return &kernelRadius; }

private:
    void transitionBuffers(ID3D12GraphicsCommandList6* cmdList, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    void resetBuffers();
    void divNumThreadsByGroupSize(StructuredBuffer* divNumThreadsByGroupSize, int groupSize);

    GridConstants gridConstants;
    
    ComputePipeline* bilevelUniformGridCP;
    ComputePipeline* surfaceBlockDetectionCP;
    ComputePipeline* surfaceCellDetectionCP;
    ComputePipeline* surfaceVertexCompactionCP;
    ComputePipeline* surfaceVertexDensityCP;
    ComputePipeline* surfaceVertexNormalCP;
    ComputePipeline* bufferClearCP;

    MeshPipeline* fluidMeshPipeline;
    
    UINT64 fenceValue = 1;
	ComPointer<ID3D12Fence> fence;

	ID3D12CommandSignature* commandSignature = nullptr;
    ID3D12CommandSignature* meshCommandSignature = nullptr;

    StructuredBuffer* positionBuffer;
    StructuredBuffer cellParticleCountBuffer;
    StructuredBuffer cellParticleIndicesBuffer;
    StructuredBuffer blocksBuffer;
    StructuredBuffer surfaceBlockIndicesBuffer;
    StructuredBuffer surfaceBlockDispatch;
    StructuredBuffer surfaceHalfBlockDispatch; // This is just 2x surfaceBlockDispatch, but saves us a round trip to the GPU to multiply by 2
    StructuredBuffer surfaceVerticesBuffer;
    StructuredBuffer surfaceVertexIndicesBuffer;
    StructuredBuffer surfaceVertDensityDispatch;
    StructuredBuffer surfaceVertDensityBuffer;
    StructuredBuffer surfaceVertexNormalBuffer;
    StructuredBuffer surfaceVertexColorBuffer;
    ComputePipeline* dispatchArgDivideCP;

    int material;
    float isovalue;
    float kernelScale;
    float kernelRadius;
};