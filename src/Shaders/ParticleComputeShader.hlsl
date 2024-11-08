cbuffer ParticleParams : register(b0) {
    float gravity;
    float deltaTime;
};

StructuredBuffer<float3> inputPositions : register(t0);       // Input positions as SRV
RWStructuredBuffer<float4x4> outputModelMatrices : register(u0); // Output model matrices as UAV

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;

    // Get the initial position
    float3 position = inputPositions[id];

    // Apply gravity to the Y position (for example, moving particles down over time)
    position.y -= gravity * deltaTime;

    // Create a model matrix with translation for the particle
    outputModelMatrices[id] = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        position.x, position.y, position.z, 1.0f
    );
}