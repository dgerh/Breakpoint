#pragma once

#include "ObjectScene.h"
#include "PBMPMScene.h"
#include "MeshShadingScene.h"
#include "../D3D/Pipeline/RenderPipeline.h"
#include "../D3D/Pipeline/ComputePipeline.h"
#include "../D3D/Pipeline/MeshPipeline.h"

class Scene {
public:
	Scene() = delete;
	Scene(Camera* camera, DXContext* context);

	RenderPipeline* getObjectWirePipeline();
	//RenderPipeline* getObjectSolidPipeline();
	//RenderPipeline* getPBMPMRenderPipeline();
	//MeshPipeline* getFluidMeshPipeline();
	//MeshPipeline* getElasticMeshPipeline();
	//MeshPipeline* getSandMeshPipeline();
	//MeshPipeline* getViscoMeshPipeline();
	//MeshPipeline* getSnowMeshPipeline();

	//void compute(float isMeshShading = false);
	//void drawPBMPM();
	//void drawFluid(unsigned int renderMeshlets, unsigned int renderOptions);
	//void drawElastic(unsigned int renderMeshlets, unsigned int renderOptions);
	//void drawSand(unsigned int renderMeshlets, unsigned int renderOptions);
	//void drawVisco(unsigned int renderMeshlets, unsigned int renderOptions);
	////void drawSnow(unsigned int renderMeshlets, unsigned int renderOptions);
	//void drawGrid();
	//void drawSpawners();
	//void drawSolidObjects();

	void releaseResources();

	/*PBMPMConstants getPBMPMConstants() { return pbmpmScene.getConstants(); }*/
	//void updatePBMPMConstants(PBMPMConstants& newConstants);

	/*float* getFluidIsovalue() { return fluidScene.getIsovalue(); }
	float* getFluidKernelScale() { return fluidScene.getKernelScale(); }
	float* getFluidKernelRadius() { return fluidScene.getKernelRadius(); }
	
	float* getElasticIsovalue() { return elasticScene.getIsovalue(); }
	float* getElasticKernelScale() { return elasticScene.getKernelScale(); }
	float* getElasticKernelRadius() { return elasticScene.getKernelRadius(); }

	float* getSandIsovalue() { return sandScene.getIsovalue(); }
	float* getSandKernelScale() { return sandScene.getKernelScale(); }
	float* getSandKernelRadius() { return sandScene.getKernelRadius(); }

	float* getViscoIsovalue() { return viscoScene.getIsovalue(); }
	float* getViscoKernelScale() { return viscoScene.getKernelScale(); }
	float* getViscoKernelRadius() { return viscoScene.getKernelRadius(); }*/

	/*float* getSnowIsovalue() { return snowScene.getIsovalue(); }
	float* getSnowKernelScale() { return snowScene.getKernelScale(); }
	float* getSnowKernelRadius() { return snowScene.getKernelRadius(); }*/

	/*unsigned int* getPBMPMSubstepCount() { return pbmpmScene.getSubstepCount(); }

	int getNumParticles() { return pbmpmScene.getNumParticles(); }*/

	bool renderToggles[5] = { 1, 1, 1, 1, 1 };

private:
	Camera* camera;

	//RenderPipeline pbmpmRP;
	//PBMPMScene pbmpmScene;

	RenderPipeline objectRPWire;
	/*ObjectScene objectSceneGrid;
	ObjectScene objectSceneSpawners;
	RenderPipeline objectRPSolid;
	ObjectScene objectSceneSolid;*/

	std::vector<SimShape> shapes = std::vector<SimShape>();

	// Fluid Mesh
	//RenderPipeline fluidRP;
	//ComputePipeline fluidBilevelUniformGridCP;
	//ComputePipeline fluidSurfaceBlockDetectionCP;
	//ComputePipeline fluidSurfaceCellDetectionCP;
	//ComputePipeline fluidSurfaceVertexCompactionCP;
	//ComputePipeline fluidSurfaceVertexDensityCP;
	//ComputePipeline fluidSurfaceVertexNormalCP;
	//ComputePipeline fluidBufferClearCP;
	//MeshPipeline fluidMeshPipeline;
	//MeshShadingScene fluidScene;

	//// Elastic Mesh
	//RenderPipeline elasticRP;
	//ComputePipeline elasticBilevelUniformGridCP;
	//ComputePipeline elasticSurfaceBlockDetectionCP;
	//ComputePipeline elasticSurfaceCellDetectionCP;
	//ComputePipeline elasticSurfaceVertexCompactionCP;
	//ComputePipeline elasticSurfaceVertexDensityCP;
	//ComputePipeline elasticSurfaceVertexNormalCP;
	//ComputePipeline elasticBufferClearCP;
	//MeshPipeline elasticMeshPipeline;
	//MeshShadingScene elasticScene;

	//// Sand Mesh
	//RenderPipeline sandRP;
	//ComputePipeline sandBilevelUniformGridCP;
	//ComputePipeline sandSurfaceBlockDetectionCP;
	//ComputePipeline sandSurfaceCellDetectionCP;
	//ComputePipeline sandSurfaceVertexCompactionCP;
	//ComputePipeline sandSurfaceVertexDensityCP;
	//ComputePipeline sandSurfaceVertexNormalCP;
	//ComputePipeline sandBufferClearCP;
	//MeshPipeline sandMeshPipeline;
	//MeshShadingScene sandScene;

	//// Visco Mesh
	//RenderPipeline viscoRP;
	//ComputePipeline viscoBilevelUniformGridCP;
	//ComputePipeline viscoSurfaceBlockDetectionCP;
	//ComputePipeline viscoSurfaceCellDetectionCP;
	//ComputePipeline viscoSurfaceVertexCompactionCP;
	//ComputePipeline viscoSurfaceVertexDensityCP;
	//ComputePipeline viscoSurfaceVertexNormalCP;
	//ComputePipeline viscoBufferClearCP;
	//MeshPipeline viscoMeshPipeline;
	//MeshShadingScene viscoScene;

	// Snow Mesh
	/*RenderPipeline snowRP;
	ComputePipeline snowBilevelUniformGridCP;
	ComputePipeline snowSurfaceBlockDetectionCP;
	ComputePipeline snowSurfaceCellDetectionCP;
	ComputePipeline snowSurfaceVertexCompactionCP;
	ComputePipeline snowSurfaceVertexDensityCP;
	ComputePipeline snowSurfaceVertexNormalCP;
	ComputePipeline snowBufferClearCP;
	MeshPipeline snowMeshPipeline;
	MeshShadingScene snowScene;*/

	RenderPipeline* currentRP;
	//ComputePipeline* currentCP;
};
