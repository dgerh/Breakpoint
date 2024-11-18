#pragma once

#include "Drawable.h"
#include "../D3D/StructuredBuffer.h"
#include "../D3D/VertexBuffer.h"
#include "../D3D/IndexBuffer.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "Geometry.h"

const unsigned int ParticleDispatchSize = 64;
const unsigned int GridDispatchSize = 8;
const unsigned int BukkitSize = 6;
const unsigned int BukkitHaloSize = 1;

struct PBMPMConstants {
	XMFLOAT2 gridSize;
	float deltaTime;
	float gravityStrength;

	float liquidRelaxation;
	float liquidViscosity;
	unsigned int fixedPointMultiplier;

	unsigned int useGridVolumeForLiquid;
	unsigned int particlesPerCellAxis;

	float frictionAngle;
	unsigned int shapeCount;
	unsigned int simFrame;

	unsigned int bukkitCount;
	unsigned int bukkitCountX;
	unsigned int bukkitCountY;
	unsigned int iteration;
	unsigned int iterationCount;
	unsigned int borderFriction;
};

struct ShapeFactory {
	XMFLOAT2 position;
	XMFLOAT2 halfSize;

	float radius;
	float rotation;
	float functionality;
	float shapeType;

	float emitMaterial;
	float emissionRate;
	float emissionSpeed;
	float padding;
};

struct PBMPMParticle {
	XMFLOAT2 position;
	XMFLOAT2 displacement;
	XMFLOAT3X3 deformationGradient;
	XMFLOAT3X3 deformationDisplacement;

	float liquidDensity;
	float mass;
	float material;
	float volume;
	
	float lambda;
	float logJp;
	float enabled;
};

struct BukkitSystem {
	unsigned int countX;
	unsigned int countY;
	unsigned int count;
	StructuredBuffer countBuffer;
	StructuredBuffer countBuffer2;
	StructuredBuffer particleData;
	StructuredBuffer threadData;
	std::array<int, 4> dispatch{ 0, 1, 1, 0 };
	std::array<int, 4> blankDispatch{ 0, 1, 1, 0 };
	std::array<int, 4> particleAllocator{ 0, 0, 0, 0 };
	StructuredBuffer indexStart;
};

class PBMPMScene : public Drawable {
public:
	PBMPMScene(DXContext* context, RenderPipeline* pipeline, ComputePipeline* compPipeline, unsigned int instanceCount);

	void constructScene();

	void compute();

	void draw(Camera* camera);

	void releaseResources();

private:



private:
	DXContext* context;
	RenderPipeline* pipeline;
	ComputePipeline* computePipeline;
	XMMATRIX modelMat;
	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> velocities;
	StructuredBuffer positionBuffer;
	StructuredBuffer velocityBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	unsigned int instanceCount;
	UINT64 fenceValue = 1;
	ComPointer<ID3D12Fence> fence;
	unsigned int indexCount = 0;
};