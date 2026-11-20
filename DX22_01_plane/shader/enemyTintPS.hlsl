#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer EnemyTintBuffer : register(b7)
{
    float4 EnemyTintColor; // RGB: target color, A: recolor strength, not transparency.
    float4 EnemyTintOptions; // X: texture-only model without a material.
};

float4 main(in PS_IN input) : SV_Target
{
    float4 surface = (EnemyTintOptions.x > 0.5 || material.TextureEnable)
        ? g_Texture.Sample(g_SamplerState, input.tex) : material.Diffuse;
    float4 color = surface * input.col;
    // Replace hue instead of multiplying it: red textures can become blue.
    // Luminance retains eyes, patterns and lighting; the square root keeps dark skins readable.
    float brightness = sqrt(saturate(dot(color.rgb, float3(0.299, 0.587, 0.114))));
    color.rgb = lerp(color.rgb, EnemyTintColor.rgb * brightness, saturate(EnemyTintColor.a));
    return color;
}
