﻿#include "bukkitInsertRootSignature.hlsl"  // Includes the ROOTSIG definition
#include "../../pbmpmShaders/PBMPMCommon.hlsl"  // Includes the TileDataSize definition

// Taken from https://github.com/electronicarts/pbmpm

// Root constants bound to b0
ConstantBuffer<PBMPMConstants> g_simConstants : register(b0);
// UAVs and SRVs;
StructuredBuffer<Particle> g_particles : register(t0);
StructuredBuffer<uint> g_particleCount : register(t1);
StructuredBuffer<uint> g_bukkitIndexStart : register(t2);
RWStructuredBuffer<uint> g_particleInsertCounters : register(u0);
RWStructuredBuffer<uint> g_particleData : register(u1);

// Compute Shader Entry Point
[numthreads(ParticleDispatchSize, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    // Check if the particle index is out of bounds
    if (id.x >= g_particleCount[0])
    {
        return;
    }

    // Load the particle
    Particle particle = g_particles[id.x];

    // Skip if the particle is disabled
    if (particle.enabled == 0.0f)
    {
        return;
    }

    // Get particle position
    float2 position = particle.position;

    // Calculate the bukkit ID for this particle
    int2 particleBukkit = positionToBukkitId(position);

    // Check if the particle is out of bounds
    if (any(particleBukkit < int2(0, 0)) ||
        uint(particleBukkit.x) >= g_simConstants.bukkitCountX ||
        uint(particleBukkit.y) >= g_simConstants.bukkitCountY)
    {
        return;
    }

    // Calculate the linear bukkit index
    uint bukkitIndex = bukkitAddressToIndex(uint2(particleBukkit), g_simConstants.bukkitCountX);
    uint bukkitIndexStart = g_bukkitIndexStart[bukkitIndex];

    // Atomically increment the particle insert counter
    uint particleInsertCounter;
    InterlockedAdd(g_particleInsertCounters[bukkitIndex], 1, particleInsertCounter);
    particleInsertCounter++;

    // Write the particle index to the particle data buffer
    g_particleData[particleInsertCounter + bukkitIndexStart] = id.x;
}