#include "VelocitySignature.hlsl"

struct Particle {
    float3 position;
    float3 previousPosition;
    float3 velocity;
    float inverseMass;
};

cbuffer SimulationParams : register(b0) {
    uint constraintCount;
    float deltaTime;
    float count;
    float breakingThreshold;
    float randomSeed;
    float3 gravity;
    float compliance;
    float numSubsteps;
};

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint particleIndex = DTid.x;
    if (particleIndex >= count) return;

    Particle p = particles[particleIndex];
    float h = deltaTime / numSubsteps;

    // Update velocity based on position change
    p.velocity = (p.position - p.previousPosition) / h;

    particles[particleIndex] = p;
}