#include "Scene.h"

Scene::Scene(Camera* p_camera, DXContext* context)
	:  camera(p_camera),
	pbmpmRP("PBMPMVertexShader.cso", "PBMPMPixelShader.cso", "PBMPMRootSignature.cso", *context, CommandListID::PBMPM_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	pbmpmScene(context, &pbmpmRP, renderToggles),
	objectRPWire("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::OBJECT_RENDER_WIRE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	objectSceneGrid(context, &objectRPWire, pbmpmScene.getSimShapes(), 1), 
	objectSceneSpawners(context, &objectRPWire, pbmpmScene.getSimShapes(), 2), 
	objectRPSolid("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::OBJECT_RENDER_SOLID_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	objectSceneSolid(context, &objectRPSolid, pbmpmScene.getSimShapes(), 0),
	// Fluid Mesh Shader Pipeline Construction
	fluidRP("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::FLUID_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidBilevelUniformGridCP("BilevelUniformGridRootSig.cso", "BilevelUniformGrid.cso", *context, CommandListID::FLUID_BILEVEL_UNIFORM_GRID_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 45, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidSurfaceBlockDetectionCP("SurfaceBlockDetectionRootSig.cso", "SurfaceBlockDetection.cso", *context, CommandListID::FLUID_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidSurfaceCellDetectionCP("SurfaceCellDetectionRootSig.cso", "SurfaceCellDetection.cso", *context, CommandListID::FLUID_SURFACE_CELL_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidSurfaceVertexCompactionCP("SurfaceVertexCompactionRootSig.cso", "SurfaceVertexCompaction.cso", *context, CommandListID::FLUID_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidSurfaceVertexDensityCP("SurfaceVertexDensityRootSig.cso", "SurfaceVertexDensity.cso", *context, CommandListID::FLUID_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidSurfaceVertexNormalCP("SurfaceVertexNormalsRootSig.cso", "SurfaceVertexNormals.cso", *context, CommandListID::FLUID_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidMeshPipeline("ConstructMeshShader.cso", "ConstructSurfaceShader.cso", "ConstructMeshRootSig.cso", *context, CommandListID::FLUID_MESH_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidBufferClearCP("bufferClearRootSignature.cso", "bufferClearComputeShader.cso", *context, CommandListID::FLUID_BUFFER_CLEAR_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	fluidScene(context, &fluidRP, &fluidBilevelUniformGridCP, &fluidSurfaceBlockDetectionCP, &fluidSurfaceCellDetectionCP, &fluidSurfaceVertexCompactionCP, 
		&fluidSurfaceVertexDensityCP, &fluidSurfaceVertexNormalCP, &fluidBufferClearCP, &fluidMeshPipeline, 0, 0.010, 5.9, 1.010),

	// Elastic Mesh Shader Pipeline Construction
	elasticRP("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::ELASTIC_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticBilevelUniformGridCP("BilevelUniformGridRootSig.cso", "BilevelUniformGrid.cso", *context, CommandListID::ELASTIC_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 45, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticSurfaceBlockDetectionCP("SurfaceBlockDetectionRootSig.cso", "SurfaceBlockDetection.cso", *context, CommandListID::ELASTIC_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticSurfaceCellDetectionCP("SurfaceCellDetectionRootSig.cso", "SurfaceCellDetection.cso", *context, CommandListID::ELASTIC_SURFACE_CELL_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticSurfaceVertexCompactionCP("SurfaceVertexCompactionRootSig.cso", "SurfaceVertexCompaction.cso", *context, CommandListID::ELASTIC_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticSurfaceVertexDensityCP("SurfaceVertexDensityRootSig.cso", "SurfaceVertexDensity.cso", *context, CommandListID::ELASTIC_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticSurfaceVertexNormalCP("SurfaceVertexNormalsRootSig.cso", "SurfaceVertexNormals.cso", *context, CommandListID::ELASTIC_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticMeshPipeline("ConstructMeshShader.cso", "ConstructSurfaceShader.cso", "ConstructMeshRootSig.cso", *context, CommandListID::ELASTIC_MESH_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticBufferClearCP("bufferClearRootSignature.cso", "bufferClearComputeShader.cso", *context, CommandListID::ELASTIC_BUFFER_CLEAR_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	elasticScene(context, &elasticRP, &elasticBilevelUniformGridCP, &elasticSurfaceBlockDetectionCP, &elasticSurfaceCellDetectionCP, &elasticSurfaceVertexCompactionCP, 
		&elasticSurfaceVertexDensityCP, &elasticSurfaceVertexNormalCP, &elasticBufferClearCP, &elasticMeshPipeline, 1, 0.010, 7.6, 1.010),

	// Sand Mesh Shader Pipeline Construction
	sandRP("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::SAND_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandBilevelUniformGridCP("BilevelUniformGridRootSig.cso", "BilevelUniformGrid.cso", *context, CommandListID::SAND_BILEVEL_UNIFORM_GRID_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 45, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandSurfaceBlockDetectionCP("SurfaceBlockDetectionRootSig.cso", "SurfaceBlockDetection.cso", *context, CommandListID::SAND_SURFACE_BLOCK_DETECTION_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandSurfaceCellDetectionCP("SurfaceCellDetectionRootSig.cso", "SurfaceCellDetection.cso", *context, CommandListID::SAND_SURFACE_CELL_DETECTION_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandSurfaceVertexCompactionCP("SurfaceVertexCompactionRootSig.cso", "SurfaceVertexCompaction.cso", *context, CommandListID::SAND_SURFACE_VERTEX_COMPACTION_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandSurfaceVertexDensityCP("SurfaceVertexDensityRootSig.cso", "SurfaceVertexDensity.cso", *context, CommandListID::SAND_SURFACE_VERTEX_DENSITY_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandSurfaceVertexNormalCP("SurfaceVertexNormalsRootSig.cso", "SurfaceVertexNormals.cso", *context, CommandListID::SAND_SURFACE_VERTEX_NORMAL_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandMeshPipeline("ConstructMeshShader.cso", "ConstructSurfaceShader.cso", "ConstructMeshRootSig.cso", *context, CommandListID::SAND_MESH_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandBufferClearCP("bufferClearRootSignature.cso", "bufferClearComputeShader.cso", *context, CommandListID::SAND_BUFFER_CLEAR_COMPUTE_ID, 
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	sandScene(context, &sandRP, &sandBilevelUniformGridCP, &sandSurfaceBlockDetectionCP, &sandSurfaceCellDetectionCP, &sandSurfaceVertexCompactionCP,
		&sandSurfaceVertexDensityCP, &sandSurfaceVertexNormalCP, &sandBufferClearCP, &sandMeshPipeline, 2, 0.010, 5.84, 1.180),

	// Visco Mesh Shader Pipeline Construction
	viscoRP("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::ELASTIC_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoBilevelUniformGridCP("BilevelUniformGridRootSig.cso", "BilevelUniformGrid.cso", *context, CommandListID::VISCO_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 45, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoSurfaceBlockDetectionCP("SurfaceBlockDetectionRootSig.cso", "SurfaceBlockDetection.cso", *context, CommandListID::VISCO_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoSurfaceCellDetectionCP("SurfaceCellDetectionRootSig.cso", "SurfaceCellDetection.cso", *context, CommandListID::VISCO_SURFACE_CELL_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoSurfaceVertexCompactionCP("SurfaceVertexCompactionRootSig.cso", "SurfaceVertexCompaction.cso", *context, CommandListID::VISCO_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoSurfaceVertexDensityCP("SurfaceVertexDensityRootSig.cso", "SurfaceVertexDensity.cso", *context, CommandListID::VISCO_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoSurfaceVertexNormalCP("SurfaceVertexNormalsRootSig.cso", "SurfaceVertexNormals.cso", *context, CommandListID::VISCO_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoMeshPipeline("ConstructMeshShader.cso", "ConstructSurfaceShader.cso", "ConstructMeshRootSig.cso", *context, CommandListID::VISCO_MESH_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoBufferClearCP("bufferClearRootSignature.cso", "bufferClearComputeShader.cso", *context, CommandListID::VISCO_BUFFER_CLEAR_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	viscoScene(context, &viscoRP, &viscoBilevelUniformGridCP, &viscoSurfaceBlockDetectionCP, &viscoSurfaceCellDetectionCP, &viscoSurfaceVertexCompactionCP,
		&viscoSurfaceVertexDensityCP, &viscoSurfaceVertexNormalCP, &viscoBufferClearCP, &viscoMeshPipeline, 3, 0.010, 4.604, 1.010),

	// Snow Mesh Shader Pipeline Construction
	/*snowRP("VertexShader.cso", "PixelShader.cso", "RootSignature.cso", *context, CommandListID::ELASTIC_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowBilevelUniformGridCP("BilevelUniformGridRootSig.cso", "BilevelUniformGrid.cso", *context, CommandListID::SNOW_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 45, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowSurfaceBlockDetectionCP("SurfaceBlockDetectionRootSig.cso", "SurfaceBlockDetection.cso", *context, CommandListID::SNOW_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowSurfaceCellDetectionCP("SurfaceCellDetectionRootSig.cso", "SurfaceCellDetection.cso", *context, CommandListID::SNOW_SURFACE_CELL_DETECTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowSurfaceVertexCompactionCP("SurfaceVertexCompactionRootSig.cso", "SurfaceVertexCompaction.cso", *context, CommandListID::SNOW_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowSurfaceVertexDensityCP("SurfaceVertexDensityRootSig.cso", "SurfaceVertexDensity.cso", *context, CommandListID::SNOW_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowSurfaceVertexNormalCP("SurfaceVertexNormalsRootSig.cso", "SurfaceVertexNormals.cso", *context, CommandListID::SNOW_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowMeshPipeline("ConstructMeshShader.cso", "ConstructSurfaceShader.cso", "ConstructMeshRootSig.cso", *context, CommandListID::SNOW_MESH_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowBufferClearCP("bufferClearRootSignature.cso", "bufferClearComputeShader.cso", *context, CommandListID::SNOW_BUFFER_CLEAR_COMPUTE_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	snowScene(context, &snowRP, &snowBilevelUniformGridCP, &snowSurfaceBlockDetectionCP, &snowSurfaceCellDetectionCP, &snowSurfaceVertexCompactionCP,
		&snowSurfaceVertexDensityCP, &snowSurfaceVertexNormalCP, &snowBufferClearCP, &snowMeshPipeline, 4, 0.010, 7.6, 1.010),*/
	
	screenQuadRP("ScreenQuadVertexShader.cso", "ScreenQuadPixelShader.cso", "ScreenQuadRootSignature.cso", *context, CommandListID::SCREEN_QUAD_RENDER_ID,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE),
	screenQuad(context, &screenQuadRP),
	currentRP(),
	currentCP()
{}

RenderPipeline* Scene::getObjectWirePipeline() {
	return &objectRPWire;
}

RenderPipeline* Scene::getObjectSolidPipeline() {
	return &objectRPSolid;
}

RenderPipeline* Scene::getPBMPMRenderPipeline() {
	return &pbmpmRP;
}

MeshPipeline* Scene::getFluidMeshPipeline() {
	return &fluidMeshPipeline;
}

MeshPipeline* Scene::getElasticMeshPipeline() {
	return &elasticMeshPipeline;
}

MeshPipeline* Scene::getSandMeshPipeline() {
	return &sandMeshPipeline;
}

MeshPipeline* Scene::getViscoMeshPipeline() {
	return &viscoMeshPipeline;
}

//MeshPipeline* Scene::getSnowMeshPipeline() {
//	return &snowMeshPipeline;
//}

RenderPipeline* Scene::getScreenQuadPipeline()
{
	return &screenQuadRP;
}

void Scene::compute(float isMeshShading) {
	pbmpmScene.compute();
	int particles = pbmpmScene.transferAndGetNumParticles();
	if (isMeshShading) {
		if (renderToggles[0]) {
			fluidScene.compute(
				pbmpmScene.getPositionBuffer(),
				particles
			);
		}
		if (renderToggles[1]) {
			elasticScene.compute(
				pbmpmScene.getPositionBuffer(),
				particles
			);
		}
		if (renderToggles[2]) {
			sandScene.compute(
				pbmpmScene.getPositionBuffer(),
				particles
			);
		}
		if (renderToggles[3]) {
			viscoScene.compute(
				pbmpmScene.getPositionBuffer(),
				particles
			);
		}
		/*if (renderToggles[4]) {
			snowScene.compute(
				pbmpmScene.getPositionBuffer(),
				particles
			);
		}*/
	}
}

void Scene::drawPBMPM() {
	pbmpmScene.draw(camera);
}

void Scene::drawFluid(unsigned int renderMeshlets, unsigned int renderOptions) {
	fluidScene.draw(camera, renderMeshlets, renderOptions);
}

void Scene::drawElastic(unsigned int renderMeshlets, unsigned int renderOptions) {
	elasticScene.draw(camera, renderMeshlets, renderOptions);
}

void Scene::drawSand(unsigned int renderMeshlets, unsigned int renderOptions) {
	sandScene.draw(camera, renderMeshlets, renderOptions);
}

void Scene::drawVisco(unsigned int renderMeshlets, unsigned int renderOptions) {
	viscoScene.draw(camera, renderMeshlets, renderOptions);
}

//void Scene::drawSnow(unsigned int renderMeshlets, unsigned int renderOptions) {
//	snowScene.draw(camera, renderMeshlets, renderOptions);
//}

void Scene::drawGrid() {
	objectSceneGrid.draw(camera);
}

void Scene::drawSpawners() {
	objectSceneSpawners.draw(camera);
}

void Scene::drawSolidObjects() {
	objectSceneSolid.draw(camera);
}

void Scene::drawScreenQuad()
{
	screenQuad.draw();
}

void Scene::releaseResources() {
	objectSceneGrid.releaseResources();
	objectSceneSpawners.releaseResources();
	objectSceneSolid.releaseResources();
	pbmpmScene.releaseResources();
	fluidScene.releaseResources();
	elasticScene.releaseResources();
	viscoScene.releaseResources();
	sandScene.releaseResources();
	pbmpmScene.releaseResources();
	pbmpmRP.releaseResources();
	//snowScene.releaseResources();
}

void Scene::updatePBMPMConstants(PBMPMConstants& newConstants) {
	pbmpmScene.updateConstants(newConstants);
}