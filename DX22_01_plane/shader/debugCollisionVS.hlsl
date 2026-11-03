#include "common.hlsl"

// Collision lines keep their debug color regardless of scene lighting.
PS_IN main(VS_IN input)
{
    PS_IN output;
    output.pos = mul(mul(mul(input.pos, World), View), Projection);
    output.col = input.col * material.Diffuse;
    output.tex = float2(0, 0);
    return output;
}
