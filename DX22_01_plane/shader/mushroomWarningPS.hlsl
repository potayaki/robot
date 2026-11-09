#include "common.hlsl"
Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 main(in PS_IN input) : SV_Target
{
    float4 surface = material.TextureEnable
        ? g_Texture.Sample(g_SamplerState, input.tex) : material.Diffuse;
    // Preserve the silhouette/alpha while making even dark textures visibly red.
    float brightness = saturate(dot(surface.rgb * input.col.rgb, float3(0.299, 0.587, 0.114)));
    return float4(0.65 + brightness * 0.35, brightness * 0.06, brightness * 0.04,
        surface.a * input.col.a);
}
