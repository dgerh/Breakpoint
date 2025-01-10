#include "ScreenQuadRootSignature.hlsl"

cbuffer UVBuffer : register(b0) {
    float2 uvs[4];
};

struct VSInput {
    float3 position : POSITION;
    uint vertexID : SV_VertexID;
};

struct PSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

[RootSignature(ROOTSIG)]
PSInput main(VSInput input) {
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.texcoord = uvs[input.vertexID];
    return output;
}