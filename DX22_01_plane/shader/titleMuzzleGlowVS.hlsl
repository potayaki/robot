#include "common.hlsl"

PS_IN main(VS_IN input) {
    PS_IN output;
    float4 center = mul(mul(input.pos, World), View);
    float radius = abs(input.tex.y) * length(mul(float3(1, 0, 0), (float3x3)World));
    float2 axis = mul(mul(float4(input.nrm.xyz, 0), World), View).xy;
    float span = length(axis);
    float2 along = span > 0.0001 ? axis / span : float2(1, 0);
    float2 across = float2(-along.y, along.x);
    float2 corner = sign(input.tex);
    center.xy += along * corner.x * max(radius, span) + across * corner.y * radius;
    output.pos = mul(center, Projection);
    output.col = input.col;
    output.tex = corner;
    return output;
}
