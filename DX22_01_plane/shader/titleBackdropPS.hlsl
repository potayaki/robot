#include "common.hlsl"
float4 main(PS_IN input) : SV_Target {
    float2 p = (input.tex - float2(0.5, 0.43)) * float2(1.15, 1.0);
    float glow = exp(-dot(p, p) * 7.0);
    float3 color = lerp(float3(0.025, 0.035, 0.047), float3(0.13, 0.17, 0.19), glow);
    return float4(color, 1);
}
