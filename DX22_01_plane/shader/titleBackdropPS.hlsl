#include "common.hlsl"

// Defocused practical lights in the workshop behind the display.
float lamp(float2 uv, float2 center, float radius) {
    float2 offset = (uv - center) * float2(1.65, 1);
    return 1.0 - smoothstep(0.0, radius, length(offset));
}

float4 main(PS_IN input) : SV_Target {
    float2 uv = input.tex;
    float2 p = uv - float2(0.5, 0.42);
    float3 color = lerp(float3(0.045, 0.065, 0.085), float3(0.21, 0.25, 0.27), exp(-dot(p, p) * 9));
    float shaft = (uv.x + uv.y * 0.27 - 0.55);
    color += float3(0.18, 0.118, 0.053) * exp(-shaft * shaft * 24) * pow(1-uv.y, 1.8);
    // Soft vertical silhouettes give the background depth without competing with the figures.
    float2 posts = (uv.x - float2(0.15, 0.79)) * float2(37, 29);
    float2 frames = exp(-posts * posts);
    float wall = 1 - smoothstep(0.58, 0.72, uv.y);
    color *= 1 - 0.35 * wall * (frames.x + frames.y);
    float warm = lamp(uv, float2(0.18, 0.37), 0.017) + lamp(uv, float2(0.81, 0.48), 0.021)
               + lamp(uv, float2(0.822, 0.48), 0.013) + lamp(uv, float2(0.90, 0.31), 0.011);
    float cool = lamp(uv, float2(0.12, 0.53), 0.024) + lamp(uv, float2(0.22, 0.58), 0.011);
    color += warm * float3(0.31, 0.18, 0.068) + cool * float3(0.09, 0.18, 0.22);
    float floorOffset = (uv.y - 0.77) * 7;
    float floorLight = exp(-floorOffset * floorOffset);
    color += floorLight * float3(0.044, 0.05, 0.05);
    return float4(color * (1 - 0.42 * smoothstep(0.28, 0.7, length(p))), 1);
}
