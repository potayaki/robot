#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 main(in PS_IN input) : SV_Target
{
    if (!material.TextureEnable)
    {
        float3 color = lerp(material.Diffuse.rgb, material.Emission.rgb, input.col.r);
        return float4(color, material.Diffuse.a * input.col.a);
    }
    return g_Texture.Sample(g_SamplerState, input.tex) * input.col * material.Diffuse;
}
