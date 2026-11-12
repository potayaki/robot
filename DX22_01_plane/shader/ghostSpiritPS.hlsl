#include "common.hlsl"

float4 main(in PS_IN input) : SV_Target
{
    float2 p = input.tex * 2.0f - 1.0f;
    float radius = length(p);
    clip(1.0f - radius);
    float glow = 1.0f - smoothstep(0.15f, 1.0f, radius);
    float core = 1.0f - smoothstep(0.0f, 0.35f, radius);
    return float4(lerp(input.col.rgb, float3(1, 1, 1), core), input.col.a * glow);
}
