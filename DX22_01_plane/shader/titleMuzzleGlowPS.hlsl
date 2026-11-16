#include "common.hlsl"

float4 main(PS_IN input) : SV_Target {
    float r2 = dot(input.tex, input.tex);
    float fade = saturate(1.0 - r2);
    return float4(input.col.rgb, input.col.a * fade * fade * fade);
}
