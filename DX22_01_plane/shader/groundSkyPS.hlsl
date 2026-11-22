#include "skyFog.hlsli"

Texture2D GroundTexture : register(t0);
SamplerState GroundSampler : register(s0);

float4 main(float4 position : SV_POSITION, float4 color : COLOR0,
            float2 uv : TEXCOORD0, float3 viewPosition : TEXCOORD1) : SV_Target
{
    float3 ground = GroundTexture.Sample(GroundSampler, uv).rgb * color.rgb;
    return float4(ApplySkyFog(ground, viewPosition, 0), 1);
}
