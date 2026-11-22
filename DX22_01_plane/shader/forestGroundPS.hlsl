#include "common.hlsl"
#include "meadowWind.hlsli"
#include "skyFog.hlsli"

Texture2D GroundTexture : register(t0);
SamplerState GroundSampler : register(s0);

float4 main(float4 position : SV_POSITION, float4 color : COLOR0,
            float2 uv : TEXCOORD0, float3 viewPosition : TEXCOORD1) : SV_Target
{
    float3 ground = color.rgb * material.Diffuse.rgb;
    float cover = saturate(color.a);
    if (material.TextureEnable) {
        float3 gravel = GroundTexture.Sample(GroundSampler, uv).rgb;
        // Keep the grain on the path while letting the fine leaves describe the meadow.
        ground *= lerp(gravel, float3(0.856, 0.843, 0.813), cover * 0.80);
    }

    // Beyond individual blades, the same gusts catch light in bands across the grass.
    float2 worldXZ = uv * 190;
    float phase = MeadowPhase();
    float gust = MeadowGust(worldXZ, phase);
    float across = dot(worldXZ, WindAcross);
    float strokes = sin(across * 0.073 + 1.8 * sin(dot(worldXZ, WindAlong) * 0.021));
    float visibleStrokes = saturate(1 - length(fwidth(worldXZ)) * 0.035);
    float wave = saturate(gust * (0.90 + 0.10 * strokes * visibleStrokes));
    float3 tint = lerp(float3(0.91, 0.94, 0.92), float3(1.23, 1.14, 1.04), wave);
    float distanceToEye = length(viewPosition);
    float strength = smoothstep(65, 465, distanceToEye) * (1 - smoothstep(1550, 2950, distanceToEye));
    ground *= lerp(1, tint, cover * strength);
    return float4(ApplySkyFog(ground, viewPosition, 600), 1);
}
