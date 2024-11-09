#include "RootSignature.hlsl"

// Define the payload structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

// Define the mesh shader
[outputtopology("triangle")]
[numthreads(1, 1, 1)]
[RootSignature(ROOTSIG)]
void main(
    uint3 threadID : SV_DispatchThreadID, 
    out vertices VertexOutput verts[3],
    out indices uint3 triangles[3])
{
    // Tell DirectX how many vertices and primitives are being output
    // The order of this call matters. See rules here: https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#setmeshoutputcounts
    SetMeshOutputCounts(3, 1);

    // Define the vertices of the triangle
    float3 vertices[3] = {
        float3(0.0f, 0.5f, 0.0f),  // Top vertex
        float3(0.5f, -0.5f, 0.0f), // Bottom-right vertex
        float3(-0.5f, -0.5f, 0.0f) // Bottom-left vertex
    };

    // Define the colors of the vertices
    float3 colors[3] = {
        float3(1.0f, 0.0f, 0.0f), // Red
        float3(0.0f, 1.0f, 0.0f), // Green
        float3(0.0f, 0.0f, 1.0f)  // Blue
    };

    // Output the vertex position and color
	for (uint i = 0; i < 3; i++)
	{
        verts[i].position = float4(vertices[i], 1.0f);
        verts[i].color = colors[i];
	}

    triangles[0] = uint3(0, 1, 2);
}
