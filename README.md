*Breakpoint* is a project created by [Daniel Gerhardt](https://www.danieljgerhardt.com/), [Dineth Meegoda](https://dinethmeegoda.com/), [Matt Schwartz](https://www.linkedin.com/in/matthew-schwartz-37019016b/), [Zixiao Wang](https://www.linkedin.com/in/zixiao-wang-826a5a255/), for CIS 5650 GPU Programming at the University of Pennsylvania.

Our project combines a novel particle simulation technique - the Position Based Material Point Method(PBMPM), developed by EA - with a state-of-the-art fluid surface construction method, using mesh shading and a bilevel-grid, all running in real time. 

To the best of our knowledge, this is the first released (and open sourced!) 3D GPU Implementation of PBMPM.

# Breakpoint

<div style="display: flex; justify-content: space-between; align-items: center; gap: 20px;">
  <img src="app/image/jellyWaterDone.gif" alt="Jelly Water Demo" height="250"/>
  <img src="app/image/particleWater.gif" alt="Particle Water Demo" height="250"/>
  <img src="app/image/viscoDemo.gif" alt="Visco Demo" height="250"/>
</div>

## Contents

- [Build the Project and Controls](#build-the-project-and-controls)
- [DirectX Core](#directx-core)
- [Position Based Material Point Method](#position-based-material-point-method)
- [Mesh shading](#mesh-shading)
- [Simulation Analysis and Performance Review](#simulation-analysis-performance-review)
- [Citations and attributions](#citations--attributions)
- [Bloopers](#bloopers-and-cool-shots)

## Using the Project and Controls

Cloning the repository and ensuring that DirectX is correctly installed should be the only steps necessary to build and run *Breakpoint* locally. The camera uses WASD for standard cardinal movement, and Space and Control for up/down movement. Press shift and rotate the mouse to rotate the camera. All mouse control of the fluid happens when right click is pressed, with extra keyboard combinations to change the functionality. Shift is for pull, alt is for grab, and no button is for push.

In order to create scenes, currently you must edit the `PBMPMScene.cpp` file and change the `createShapes()` function. When adding shapes, make sure to adhere to the order of arguments in the Shape struct defined in `PBMPMScene.h`. Whichever particles you want to render must also be set in the `renderToggles` array defined at the top of the function. 

By default, Fluid and Elastic are set to render with a fluid waterfall and two jelly cubes spawning.

In order to change rendering modes, simulation parameters, or mesh shading parameters, use the imgui window in the application.

#### Building in VS Code
- Clone the repository
- From the command pallete (Ctrl + Shift + P), run `Debug: Select and Start Debugging > Release` to build and run the release build of the project.

## DirectX Core

We built our project on top of the DirectX 12 graphics API, creating our own engine infrastructure with guidance from the DX documentation, samples, and tutorial series by Ohjurot. Our engine includes wrapper classes for central DirectX concepts such as structured buffers and descriptor heaps. It also provides scene constructs with support for standard vertex render pipelines, mesh-shading pipelines, and compute pipelines. With these, we can render meshes from OBJ files, PBMPM particles, and mesh-shaded fluid surfaces. The default scene includes a first person camera and mouse interaction with the PBMPM particle simulation.

For the graphical interface (which includes interactive sliders for the simulation and rendering parameters), we used ImGUI.

## Position Based Material Point Method

*Breakpoint* includes a 3D implemention of EA's Position Based Material Point Method. The public repository for PBMPM is a 2D WebGPU version, and there is an EA in-house 3D version on the CPU. *Breakpoint* is the first instance of PBMPM in DirectX on the GPU in 3D. 

For particle simulations, the Material Point Method(MPM) is currently one of the most widely used integration techniques. This is for a couple reasons - namely that it is easily implemented, and it is easy to scale to incorporate numerous varied material types. However, it is not entirely stable, and certain behaviors like compressing a liquid to 0 volume or vast force differences can cause strange behavior. In real time environments like games, particle simulations should be real time, accurate, and behave as expected with the user's inputs. Ensuring stability across user inputs is one of the advantages of PBMPM, the Position Based Material Point Method. As can be discerned from the name, it is a modification upon MPM. It utilizes a semi-implicit compliant constraint, is stable at any time step, and does not add complex implementation overhead to MPM. Our work moved the simulation to 3D on the GPU in a custom DirectX 12 engine.

For a detailed overview from the author of PBMPM, Chris Lewin, please refer to the [repository](https://github.com/electronicarts/pbmpm) and his [SIGGRAPH presentation](https://www.youtube.com/watch?v=-ERzqNADJ8w&t=5s) from this year.

The pseudocode algorithm for PBMPM is as follows:

<p align="center">
  <img src="app/image/pbmpmalgo.png" alt="PBMPM Algorithm" />
</p>

As highlighted in red, the approach uses constraints like in PBD for rigid and soft body simulations that are seen often in modern games. However, the overall algorithmic approach is very similar to standard MPM, where there is a background grid that stores information for the simulation. 

### Materials

Most of the materials we have are based on the original public PBMPM paper 2D materials implementations (liquid, sand, elastics, Visco). However, the snow is mainly based on Disney's work about [A material point method for snow simulation](https://media.disneyanimation.com/uploads/production/publication_asset/94/asset/SSCTS13_2.pdf). The snow material implementation combines elastic deformation (reversible) with plastic deformation (permanent) using a multiplicative decomposition approach. This creates a material that can both temporarily deform and permanently change shape. 

#### Liquid
The liquid material implementation in our MPM system combines volume preservation with viscosity control to simulate fluid behavior. The implementation is primarily based on the Position Based Fluids (PBF) approach, adapted for the MPM grid-based framework.

<p align="center">
  <img src="app/image/fluidParticles.gif" alt="watergif"/>
</p>


The main liquid update happens in the constraint solving phase:
``` 
if (particle.material == MaterialLiquid)
{
    // Simple liquid viscosity: just remove deviatoric part of the deformation displacement
    float3x3 deviatoric = -1.0 * (particle.deformationDisplacement + transpose(particle.deformationDisplacement));
    particle.deformationDisplacement += g_simConstants.liquidViscosity * 0.5 * deviatoric;

    float alpha = 0.5 * (1.0 / liquidDensity - tr3D(particle.deformationDisplacement) - 1.0);
    particle.deformationDisplacement += g_simConstants.liquidRelaxation * alpha * Identity;
}
```

#### Sand

The sand implementation is based on the [Drucker-Prager plasticity model](https://dl.acm.org/doi/10.1145/2897824.2925906). 

<p align="center">
  <img src="app/image/sandParticles.gif" alt="sandgif" />
</p>

The main update loop handles the elastic-plastic decomposition of deformation and volume preservation.

```
    // Step 1: Compute total deformation gradient
    float3x3 F = mul(Identity + particle.deformationDisplacement, particle.deformationGradient);
    
    // Step 2: Perform SVD decomposition
    SVDResult svdResult = svd(F);
    
    // Step 3: Safety clamp singular values
    svdResult.Sigma = clamp(svdResult.Sigma,
        float3(0.1, 0.1, 0.1),
        float3(10000.0, 10000.0, 10000.0));
        
    // Step 4: Volume preservation
    float df = det(F);  // Current volume
    float cdf = clamp(abs(df), 0.4, 1.6);  // Clamp volume change
    float3x3 Q = mul((1.0f / (sign(df) * cbrt(cdf))), F);  // Volume preserving target
    
    // Step 5: Elastic-plastic blending
    float3x3 elasticPart = mul(mul(svdResult.U, diag(svdResult.Sigma)), svdResult.Vt);
    float3x3 tgt = alpha_blend * elasticPart + (1.0 - alpha_blend) * Q;
```

#### Visco

The Visco material model in this project represents a highly viscous, non-Newtonian fluid-like material that bridges the gap between purely elastic solids and fully fluid materials. It is intended for scenarios where you want to simulate materials that flow under prolonged stress but still maintain some structural integrity under short loading times—such as pitch, wax, or thick mud. By carefully tuning the parameters, you can achieve a range of behaviors from a slow-creeping putty to a thick slurry.

<p align="center">
  <img src="app/image/viscoParticles.gif" alt="visco gif" />
</p>

```
    // Step 1: Compute deformation gradient
    float3x3 F = mul(Identity + particle.deformationDisplacement, particle.deformationGradient);
    
    // Step 2: SVD decomposition
    SVDResult svdResult = svd(F);
    
    // Step 3: Get current volume
    float J = svdResult.Sigma.x * svdResult.Sigma.y * svdResult.Sigma.z;
    
    // Step 4: Clamp deformation within yield surface
    svdResult.Sigma = clamp(svdResult.Sigma,
        float3(1.0 / yieldSurface, 1.0 / yieldSurface, 1.0 / yieldSurface),
        float3(yieldSurface, yieldSurface, yieldSurface));
    
    // Step 5: Volume preservation
    float newJ = svdResult.Sigma.x * svdResult.Sigma.y * svdResult.Sigma.z;
    svdResult.Sigma *= pow(J / newJ, 1.0 / 3.0);
    
    // Step 6: Update deformation gradient
    particle.deformationGradient = mul(mul(svdResult.U, diag(svdResult.Sigma)), svdResult.Vt);
```

#### Elastic

The Elastic material in this project simulates a solid-like material that deforms under load but returns to its original shape once forces are removed, analogous to rubber or soft metals (in their elastic range). It is the simplest and most fundamental of the material implemented, serving as a baseline for understanding more complex behaviors like plasticity or viscosity.

<p align="center">
  <img src="app/image/elasticParticles.gif" alt="elastic" />
</p>

Unlike plastic or granular materials, the Elastic material does not accumulate permanent changes. Once the external force or displacement is removed, the elastic material returns to its initial state. If you stretch it and let go, it rebounds. So there is no need to update and accumulate the deform shape, the main update loop is in the particles update section.
```
    // Step 1: Compute deformation gradient
    float3x3 F = mul(Identity + particle.deformationDisplacement, particle.deformationGradient);
    
    // Step 2: SVD decomposition
    SVDResult svdResult = svd(F);

    // Step 3: Volume preservation
    float df = det(F);
    float cdf = clamp(abs(df), 0.1, 1000.0);
    float3x3 Q = mul((1.0 / (sign(df) * cbrt(cdf))), F);
    
    // Step 4: Combine rotation and volume preservation
    float alpha = elasticityRatio;
    float3x3 rotationPart = mul(svdResult.U, svdResult.Vt);
    float3x3 targetState = alpha * rotationPart + (1.0 - alpha) * Q;
    
    // Step 5: Update displacement
    float3x3 invDefGrad = inverse(particle.deformationGradient);
    float3x3 diff = mul(targetState, invDefGrad) - Identity - particle.deformationDisplacement;
    particle.deformationDisplacement += elasticRelaxation * diff;
```

#### Snow

The Snow material model is based on the elastoplastic framework described in [Stomakhin et al. 2013, A Material Point Method for Snow Simulation](https://media.disneyanimation.com/uploads/production/publication_asset/94/asset/SSCTS13_2.pdf). This model treats snow as a porous, elastoplastic medium that resists deformation elastically up to certain critical thresholds, after which it undergoes permanent (plastic) rearrangements. The approach enables simulating both firm, rigid snowpacks and flowing avalanches within a unified formulation.

<p align="center">
  <img src="https://github.com/user-attachments/assets/31ac755b-1a6d-47b0-9b45-f3190500acb4" alt="snowgif" />
  <br>
</p>

Unlike purely elastic materials, snow undergoes permanent changes when overstressed. The deformation gradient F is multiplicatively decomposed into elastic (Fe) and plastic (Fp) components:

* Elastic Component (Fe): Captures recoverable deformations. When loads are removed, Fe tends toward the identity, restoring the material to its original shape (as long as the deformation stays under critical thresholds).
* Plastic Component (Fp): Records permanent rearrangements of the snow’s internal structure. Changes in Fp are not reversed upon unloading, reflecting crushed bonds or irreversibly compacted microstructure.

```
    // Step 1: SVD decomposition of deformation gradient
    SVDResult svdResult = svd(particle.deformationGradient);

    // Step 2: Elastic bounds
    float3 elasticSigma = clamp(svdResult.Sigma,
        float3(1.0f - criticalCompression, 1.0f - criticalCompression, 1.0f - criticalCompression),
        float3(1.0f + criticalStretch, 1.0f + criticalStretch, 1.0f + criticalStretch));

    // Step 3: Volume-based hardening
    float Je = elasticSigma.x * elasticSigma.y * elasticSigma.z;
    float hardening = exp(hardeningCoeff * (1.0f - Je));

    // Step 4: Elastic-plastic decomposition
    float3x3 Fe = mul(mul(svdResult.U, diag(elasticSigma)), svdResult.Vt);
    float3x3 FeInverse = mul(mul(svdResult.U, diag(1.0 / elasticSigma)), svdResult.Vt);
    float3x3 Fp = mul(particle.deformationGradient, FeInverse);
```
## Mesh Shading

<p align="center">
  <img src="app/image/alembic.gif" alt="Meshlets" />
  <br>
  <i>Fluid data rendered with mesh shading, our implementation.</i>
  </br>
  <i>(data and technique from Nishidate et al)</i>
</p>

<p align="center">
  <img src="app/image/fluid_overview.png" alt="Fluid surface overview" />
  <br>
  <i>Fluid surface reconstruction - Nishidate et al.</i>
</p>

We used a novel approach for constructing the fluid surface's polygonal mesh, via a GPU-accelerated marching cubes based algorithm that utilizes mesh shaders to save on memory bandwidth. For the unfamiliar, mesh shading is a relatively new technology that replaces the vertex, primitive assembly, and other optional stages of the graphics pipeline with a single, programmable, compute-like stage. The mesh-shading stage provides ultimate flexibility and opportunity to optimize and reduce memory usage, at the cost of increased implementation complexity. Notably, a mesh shader can only output a maximum of 256 vertices per "workgroup." This means that each workgroup of a mesh shader must coordinate, outputting individual "meshlets" to combine into the overall mesh:


<p align="center">
  <img src="app/image/meshlets.png" alt="Meshlets" />
  <br>
  <i>Meshlets to meshes - Nishidate et al.</i>
</p>

Our specific usage of mesh shading follows [Nisidate et al.](https://dl.acm.org/doi/10.1145/3651285), which creates a bilevel uniform grid to quickly scan over fluid particles from coarse-to-fine, in order to build a density field. This density field is the input to the mesh shader, which runs marching cubes per grid cell to create a fluid surface mesh. By aligning each mesh shading workgroup with half a grid "block," this approach reduces vertex duplication at block-boundaries while staying under the 256-vertex limit. The general flow looks like this:

<p align="center">
  <img src="app/image/fluidmesh_flowchart.png" alt="flowchart" />
  <br>
  <i>Technique overview - Nishidate et al.</i>
</p>

The above stages manifest as 6 compute passes and a mesh shading pass, which can be described as:
1. Building the grids (putting particles into cells and blocks)
2. Detecting which blocks are "surface" blocks
3. Detecting which cells are "surface" cells
4. Compacting vertices of all surface cells into a non-sparse array
5. Computing the density of particles at each surface vertex
6. Computing the normals at each surface vertex
7. (Mesh shading) Using marching cubes to triangulate the particle density field, then fragment shading (which uses the normals).

In the course of implementing these passes, we found many opportunities for significant performance improvements based on core concepts taught in CIS 5650. To illustrate this, compare the average compute time (in milliseconds, averaged over 1,000 frames) for each step listed above between our implementation and the original:

| Algorithm step              | Nishidate et al. | Ours  |
|-----------------------------|------------------|-------|
| Construct bilevel grid      | 0.297            | 0.107 |
| Detect surface blocks       | 0.099            | 0.006 |
| Detect surface cells        | 2.074            | 0.083 |
| Compact surface vertices    | 1.438            | 0.072 |
| Surface vertex density      | 0.391            | 0.561 |
| Surface vertex normal       | 0.038            | 0.262 |
| Mesh shading                | 0.611            | 1.038 |
| **Total:**                  | **4.948**        | **2.129** |

<p align="center">
  <img src="app/image/mesh_perf_chart.svg" alt="flowchart" />
  <br>
  <i>Performance comparison</i>
  <br>
  <i>(Tested on: Windows 10 22H2, Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz, NVIDIA GeForce RTX 2060)</i>
</p>


Let's discuss each step in the technique and analyze the optimizations that account for the differences in computation time:

### Construct the bilevel grid

In this step, particles are placed into blocks (the coarse structure) and cells (the fine structure). Threads are launched for each particle, and the particle's position determines its host cell. The first particle in each cell increments its block's count of non-empty cells, *as well as its neighboring blocks'*. In the original paper, this first cell iterates over 27 neighboring blocks and uses highly-divergent logic to narrow down to a maximum of 8 potential neighbors close enough to be affected.

In our implementation, we use clever indexing math to calculate the iteration bounds for the exact 8 neighboring blocks a-priori, avoiding much of the divergent logic.

<p align="center">
  <img src="app/image/indexingillustration.png" alt="indexmath" />
  <br>
  <i>The light blue cell is on the positive X edge and negative Y edge of the block, thus (1, -1). Each closest neighbor (red) can be derived via these offsets. The same approach extends to 8 neighbors in 3D. </i>
</p>

We can take a vector from the center of a block to the cell represented by a given thread, and then remap that distance such that cells on the border of a block are ±1, and anything inside is 0. Not only does this tell us whether or not a cell is on a block boundary, it also gives us the directions we need to search for the 8 neighboring blocks 

```GLSL
    // (±1 or 0, ±1 or 0, ±1 or 0)
    // 1 indicates cell is on a positive edge of a block, -1 indicates cell is on a negative edge, 0 indicates cell is not on an edge
    // This should work for both even and odd CELLS_PER_BLOCK_EDGE
    float halfCellsPerBlockEdgeMinusOne = ((CELLS_PER_BLOCK_EDGE - 1) / 2.0);
    int3 edge = int3(trunc((localCellIndices - halfCellsPerBlockEdgeMinusOne) / halfCellsPerBlockEdgeMinusOne));

    ...

    int3 globalNeighborBlockIndex3d = clamp(blockIndices + edge, int3(0, 0, 0), gridBlockDimensions - 1);
    int3 minSearchBounds = min(blockIndices, globalNeighborBlockIndex3d);
    int3 maxSearchBounds = max(blockIndices, globalNeighborBlockIndex3d);
```

As you can see in the above results table, this single difference nearly tripled the performance of this step when compared to the original. A great example of the power of thread convergence!

### Detect surface blocks

In this step, we take the array of blocks, detect which are surface blocks (`nonEmptyCellCount > 0`), and put the indices of those surface blocks into a new, compact array. Each thread acts as a single block. The paper accomplishes this by having each surface-block thread atomically add against an index, and using that unique index to write into the output surface block indices array.

Both the issue with and solution to this approach were immediately obvious to us: 

The issue - heavy use of atomics creates high contention and is very slow. 

The solution - stream compaction! Stream compaction is widely used to do precisely this task: filter on an array and compress the desired entries into an output array.

<p align="center">
  <img src="app/image/streamcompaction.png" alt="streamcompaction" />
  <br>
  <i>Stream compaction illustrated</i>
</p>

With [parallel prefix scan](https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda), 0 atomic operations are necessary. However, given our choice of framework and the lack of a library like Thrust, implementing a prefix scan in the timeframe of this project was out of scope. Instead, we opted for a simple, yet highly effective, wave-intrinsic approach, [based on this article](https://interplayoflight.wordpress.com/2022/12/25/stream-compaction-using-wave-intrinsics/). The gist is, each wave coordinates via intrinsics to get unique write-indices into the output array, needing only 1 atomic operation per-wave to sync with the other waves. With 32 threads per wave, this is a 32x reduction in atomic usage! The effect may amplify further, however, since there's much less chance of resource contention with such a reduction in atomic usage.

As such, it's no surprise that our implementation of this stage was over 16x faster than the original!

### Detect Surface Cells

In this next step, we move from the coarse-grid to the fine. Each thread in this pass, representing a single cell in a surface block, checks its neighboring 27 cells to determine if it is a surface cell. Surface cells are those which are neither completely surrounded by filled cells, nor by completely empty cells (itself included).

The great time-sink in the original paper's implementation is its heavy global memory throughput. Since each cells needs to reference all of its neighbors, that means each thread does 27 global memory accesses. Ouch! We realized, however, that many of these accesses are redundant. Consider two adjacent cells; each has 27 neighbors, but each shares 9 of those neighbors! The optimization opportunity improves exponentially when you consider that we're analyzing *blocks* of cells, with nearly complete neighbor overlap.

The solution, then, is to pull all cell data for a block into groupshared memory. This way, for a given block of 64 cells (4 x 4 x 4), instead of doing 27 * 64 = 1728 global memory reads, we only need to do 125 reads (5 x 5 x 5). Each cell still looks up neighboring information, but from shared memory instead of global memory. The indexing is somewhat complex to make sure that 64 threads can efficiently pull data for 125 cells, but we can actually repurpose the trick from the bilevel grid construction to get the right iteration bounds!

And, once again, looking at the performance results above, we see a **massive** 25x increase in performance.

### Compact Surface Vertices

This is exactly the same idea as the wave intrinsic optimization done in detecting surface blocks, but with the surface vertices! Because there are so many more vertices than surface blocks, the contention with atomic usage is exacerbated, thus the 20x performance increase with our approach!

### The rest

Aside from these major optimizations, there were also several smaller ones that we'll mention briefly:
- The original paper uses one-dimensional workgroups. This results in a few places where expensive modular arithmetic must be used to convert from a 1D thread ID to a 3D index. By using a 3D workgroup, which better aligns with the underlying model, we can avoid some (but not all) of that modular arithmetic
- Rather than projecting each fluid mesh fragment to screen space in the fragment shader, we do that in the mesh shading step per-vertex and allow it to be interpolated.
- We also avoided divergent control flow logic in deeply nested loops by precalculating iteration bounds (and pre-clamping to grid boundaries).

Now for the elephant in the room: why are the last 3 stages slower in our implementation? The short answer is: we're not exactly sure why, but we have theories.

- Surface vertex density and surface normals: our implementations for these passes are nearly identical to the original paper's. If anything, minor optimizations in these passes should have results in slightly faster times. 
  - Our best guess for the discrepancies are that our wave-intrinsic optimization in the vertex compaction stage somehow results in a less-ideal memory layout for following stages. 
  - These stages also appear to be highly sensitive to workgroup sizes, so it's possible that we just did not find the optimal configuration for our hardware. 
  - Lastly, some differences may be due to engine differences (being new to DirectX, we may not have written optimal engine code), and graphics API platforms (Vulkan + GLSL vs. DirectX + HLSL).
</br>

- Mesh shading: again, our implementation is similar to the original paper's. However, some steps in the original implementation are actually undefined behavior, forcing us to diverge slightly (these behaviors). 
  - Most notably, since mesh output sizes (number of vertices and primitives) must be declared before writing to them, we needed to use extra shared memory to store our vertex data first, then later write them to global memory.
  - Where the original implementation does use shared memory, they do not synchronize the workgroup after writing.
  - There are a few early returns in the original implementation. We believe this is also undefined behavior in mesh shaders, with certain restrictions. We tried our best to accomplish the same computation-savings by similar means, but it may not be comparable.
  - Again, workgroup-size sensitivity and potentially engine differences.

# Simulation Analysis, Performance Review

## ImGui Toggles

We have implemented some helpful debug views and quick switch parameters with ImGui for tuning.

### Simulation Parameters

![](app/image/simparams.png)

These parameters tune how the particles behave, and the speed and accuracy of the simulation.

### Mesh Shading Parameters

![](app/image/meshshadeparams.png)

- Isovalue: determines what fluid density constitutes the "surface."
- Kernel scale: generally speaking, controls the "blobiness" of the fluid.
- Kkernel radius: controls the search radius for neighboring cells in the algorithm.

### Render Parameters

![](app/image/renderparams.png)

There are three render modes to toggle between, and some debug view options.

![](app/image/griddebugview.png)

Toggling on the grid view will color the different simulation shapes(colliders, drains, emitters, the grid). There are also render modes for particles with mesh shading, and just mesh shading, pictured here respectively: 

![](app/image/rendermode2.png)

![](app/image/rendermode3.png)

Lastly, there is a toggle to see the meshlets in the mesh shader, or toon shading:

![](app/image/meshletrendertoggle.png)

## Position Based Material Point Method Performance

There are a number of parameters that affect how PBMPM performs, due to the complexity of the simulation algorithm. The following is a list of performance tests for PBMPM, analyzing the various parameters and attributes of the particle simulation. For the setup, unless otherwise stated the simulation parameters are as shown above in the demonstration of the ImGUI toggles. To isolate the performance of PBMPM as much as possible, mesh shading was not on for these tests and particles were rendered as unlit low resolution spheres. 

The iteration count, subset count, and particle count tests were performed in release mode on a personal laptop with Windows 10 Pro, Ryzen 9 5900X 12 Core @ 3.7GHz 32GB, RTX 3070 8GB. The remainder of the PBMPM tests were performed in release mode on a personal laptop with Windows 23H2, an AMD Ryzen 9 7940HS @ 4GHz 32GB, and a RTX 4070 8 GB.

The primary 2 are the iteration count and substep count. The substep count runs bukkiting and emission as well as g2p2g for each update within a frame. The iteration count is a subroutine of substep count that determines how many times g2p2g is run within each substep. The two of these have major impacts on performance.

<p align="center">
  <img src="app/image/itercount.png" alt="itercount" />
</p>

For this test, we used a scene of 107,217 fluid particles (from a 20 x 20 x 20 inital spawning volume). These parameters, along with many others that are tied to the simulation, are at the user's discretion to choose between performance, stability, and accuracy. Having a higher iteration and substep count will increase the stability and accuracy at the cost of performance. A nice sweet spot is what we used for our basic testing setup.

<p align="center">
  <img src="app/image/particlecount.png" alt="particlecount" />
</p>

The simulation has an expected albeit high falloff in performance as particle count increases. The large memory overhead creates a big disparity in performance between high and low particle counts. This is the biggest determining factor in performance, because the number of dispatches is based in thef number of bukkits containing particles.

<p align="center">
  <img src="app/image/gridsize.png" alt="gridsize" />
</p>

The grid size performance is connected to the number of bukkits within the grid. Generally as the grid grows, performance decreases, but due to the limit of memory in the 3d implementation, the grid could not be stably tested past 256x256x256. 32x32x32 is likely slower than 64x64x64 because it is more likely particles were reaching edge conditions and causing branching behavior due to border friction.

<p align="center">
  <img src="app/image/griddispatch.png" alt="griddispatch" />
</p>

Grid dispatch size is the dispatch size for any compute shader that runs per grid cell. It didn't have a noticable performance impact outside the margin of error, and the simulation did not function when the grid dispatch was not between 4 and 10.

<p align="center">
  <img src="app/image/particledispatch.png" alt="particledispatch" />
</p>

Particle dispatch size, similarly to grid dispatch, is the disaptch size for any compute shader that runs per particle. Performance decreased when particle dispatch size increased. This was a marginal decrease. It is likely caused by larger workgroups increasing the number of threads within a single workgroup that need to access memory.

<p align="center">
  <img src="app/image/bukkithalosize.png" alt="cellaxis" />
</p>

Bukkit size and bukkit halo size determine the size of the cells that particles are grouped into and how far particles can look around them to determine the influence of surrounding cells respectively. Due to the constraints of memory usage in 3D, the 4 configurations above are the only viable ones that allow the shaders to run. This is due to the shared memory limitation of DirectX 12, which is 32kb in compute shaders. Decreasing the bukkit size increases performance, as does bukkit halo size. The halo size has a greater effect, which is expected since the halo size increase causes a vast increase in the memory access of each thread. However, it is not advised to reduce these past 2 for bukkit size and 1 for halo size, since the simulation does not perform stably below these values. Ideally, these values could be increased, but because of shared memory limitations, they cannot be in the current implementation. One avenue of investigation is determining whether a greater bukkit size with no shared memory would yield performance improvements.

<p align="center">
  <img src="app/image/materials.png" alt="materials" />
</p>

The above chart analyze the average FPS lost when adding 1 cube of 2000 particles of varying material types. The average FPS lost corresponds to the complexity of the simulation equations for each of the materials. Adding various materials increases thread divergence and computational weight at the cost of increased realism and scene depth.

## Mesh Shading Performance

For a detailed analysis of the following performance data, see [this section](#Fluid-Mesh-Shading).

| Algorithm step              | Nishidate et al. | Ours  |
|-----------------------------|------------------|-------|
| Construct bilevel grid      | 0.297            | 0.107 |
| Detect surface blocks       | 0.099            | 0.006 |
| Detect surface cells        | 2.074            | 0.083 |
| Compact surface vertices    | 1.438            | 0.072 |
| Surface vertex density      | 0.391            | 0.561 |
| Surface vertex normal       | 0.038            | 0.262 |
| Mesh shading                | 0.611            | 1.038 |
| **Total:**                  | **4.948**        | **2.129** |

<p align="center">
  <img src="app/image/mesh_perf_chart.svg" alt="flowchart" />
  <br>
  <i>Performance comparison</i>
  <br>
  <i>(Tested on: Windows 10 22H2, Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz, NVIDIA GeForce RTX 2060)</i>
</p>


Helpful resources: 
- [PBMPM](https://www.ea.com/seed/news/siggraph2024-pbmpm)
- [Fluid Mesh Shading Repository](https://github.com/yknishidate/mesh_shader_surface_reconstruction/tree/main/shader)
- For the DX12 basics and compointer class, we used this great [tutorial series!]( https://github.com/Ohjurot/D3D12Ez)
- For the DescriptorHeap class and management, we used this [resource](https://github.com/stefanpgd/Compute-DirectX12-Tutorial/).

# Citations / Attributions

Thank you to Chris Lewin, Ishaan Singh, and Yuki Nishidate for corresponding with us and offering implementation advice throughout the course of this project.

```
@article{10.1145/3651285,
    author = {Nishidate, Yuki and Fujishiro, Issei},
    title = {Efficient Particle-Based Fluid Surface Reconstruction Using Mesh Shaders and Bidirectional Two-Level Grids},
    year = {2024},
    issue_date = {May 2024},
    publisher = {Association for Computing Machinery},
    address = {New York, NY, USA},
    volume = {7},
    number = {1},
    url = {https://doi.org/10.1145/3651285},
    doi = {10.1145/3651285},
    journal = {Proc. ACM Comput. Graph. Interact. Tech.},
    month = {may},
    articleno = {1},
    numpages = {14},
}
```

```
Chris Lewin. [A Position Based Material Point Method](https://www.ea.com/seed). ACM SIGGRAPH 2024.
```

```
@article{10.1145/2897824.2925906,
    author = {Kl\'{a}r, Gergely and Gast, Theodore and Pradhana, Andre and Fu, Chuyuan and Schroeder, Craig and Jiang, Chenfanfu and Teran, Joseph},
    title = {Drucker-prager elastoplasticity for sand animation},
    year = {2016},
    issue_date = {July 2016},
    publisher = {Association for Computing Machinery},
    address = {New York, NY, USA},
    volume = {35},
    number = {4},
    issn = {0730-0301},
    url = {https://doi.org/10.1145/2897824.2925906},
    doi = {10.1145/2897824.2925906},
    abstract = {We simulate sand dynamics using an elastoplastic, continuum assumption. We demonstrate that the Drucker-Prager plastic flow model combined with a Hencky-strain-based hyperelasticity accurately recreates a wide range of visual sand phenomena with moderate computational expense. We use the Material Point Method (MPM) to discretize the governing equations for its natural treatment of contact, topological change and history dependent constitutive relations. The Drucker-Prager model naturally represents the frictional relation between shear and normal stresses through a yield stress criterion. We develop a stress projection algorithm used for enforcing this condition with a non-associative flow rule that works naturally with both implicit and explicit time integration. We demonstrate the efficacy of our approach on examples undergoing large deformation, collisions and topological changes necessary for producing modern visual effects.},
    journal = {ACM Trans. Graph.},
    month = jul,
    articleno = {103},
    numpages = {12},
    keywords = {sand, granular, elastoplasticity, MPM, APIC}
}
```

```
Stomakhin, A., Schroeder, C., Chai, L., Teran, J., Selle, A. 2013. A Material Point Method for Snow Simulation. ACM Trans. Graph. 32, 4, Article 102 (July 2013), 12 pages. DOI = 10.1145/2461912.2461948
http://doi.acm.org/10.1145/2461912.2461948.
```

## PBMPM BSD-3-Clause License:
```
Copyright (c) 2024 Electronic Arts Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the name of Electronic Arts, Inc. ("EA") nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

# Bloopers and Cool Shots

<p align="center">
  <i>Inside the mesh-shaded fluid in PIX debug view</i>
  </br>
  <img src="app/image/indamesh.png" alt="inside the mesh" />
</p>


<p align="center">
  <img src="app/image/indamesh2.png" alt="inside the mesh 2" />
</p>

<p align="center">
  <i>Celebration splash when we first got 2D working</i>
  </br>
  <img src="app/image/firstFluid.gif" alt="First fluid, 2D" />
</p>

<p align="center">
  <img src="app/image/waterfall.gif" alt="More 2D fluid" />
</p>

<p align="center">
  <i>Accidental meshlet couch, forgot the depth buffer!</i>
  </br>
  <img src="app/image/meshcouch.png" alt="More 2D fluid" />
</p>

<p align="center">
  <i>First attempt at going from 2D to 3D PBMPM</i>
  </br>
  <img src="app/image/startOf3D.gif" alt="First attempt at porting 2D to 3D" />
</p>

<p align="center">
  <i>Some angry particles</i>
  </br>
  <img src="app/image/swirly-wirly.gif" alt="Some angry particles" />
</p>

<p align="center">
  </br>
  <img src="app/image/evilwhirl.gif" alt="Evil whirl" />
</p>
