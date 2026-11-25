#include "common.hlsl"

float4 main(PS_IN input) : SV_Target
{
    float2 p = input.tex * 2.0 - 1.0;
    float radius = length(p);
    float edge = max(fwidth(radius), 0.003);
    float core = 1.0 - smoothstep(0.44 - edge * 0.5, 0.44 + edge * 0.5, radius);

    // 深い緑の縁と金緑色に光る中心で、晴れた草地でも玉の形を残す。
    float3 rim = float3(0.025, 0.32, 0.16);
    float3 emerald = float3(0.24, 0.95, 0.32);
    float inside = 1.0 - smoothstep(0.29, 0.43, radius);
    float shine = 1.0 - smoothstep(0.055, 0.34, length(p - float2(-0.065, -0.075)));
    float3 coreColor = lerp(lerp(rim, emerald, inside), float3(0.97, 1.0, 0.45), shine);

    // 光だけの位置は背景を透過して加算し、白熱した芯では元の色を保つ。
    float glow = 1.0 - smoothstep(0.33, 1.0, radius);
    float3 halo = float3(0.20, 0.76, 0.28) * (glow * glow * 0.56 * input.col.a);
    return float4(coreColor * core + halo * (1.0 - core), core);
}
