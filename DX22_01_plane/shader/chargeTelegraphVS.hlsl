#include "common.hlsl"

PS_IN main(VS_IN input)
{
    PS_IN output;
    output.pos = mul(mul(mul(input.pos, World), View), Projection);
    output.col = input.col; // Keep warning red visible in every sky/lighting preset.
    output.tex = 0;
    return output;
}
