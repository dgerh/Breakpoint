#include "ScreenQuadRootSignature.hlsl"

Texture2D tex : register(t0);
SamplerState mySampler : register(s0);

struct PSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

[RootSignature(ROOTSIG)]
float4 main(PSInput input) : SV_Target {
    // Sample the texture
    float4 color = tex.Sample(mySampler, input.texcoord);
    return color;
}