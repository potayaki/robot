#include "common.hlsl"
#include "skyFog.hlsli"

Texture2D SceneryTexture : register(t0);
SamplerState ScenerySampler : register(s0);

float4 main(float4 position : SV_POSITION, float4 color : COLOR0,
            float2 uv : TEXCOORD0, float3 viewPosition : TEXCOORD1) : SV_Target
{
    color *= material.Diffuse;
    if (material.TextureEnable) color *= SceneryTexture.Sample(ScenerySampler, uv);
    // Keep the immediate clearing crisp; fade the next room into the current sky.
    return float4(ApplySkyFog(color.rgb, viewPosition, 600), color.a);
}
