#include "common.hlsl"
struct DioramaPixel {
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
DioramaPixel main(VS_IN input) {
    DioramaPixel output;
    output.position = mul(mul(mul(input.pos, World), View), Projection);
    // Inverse transpose, including non-uniform prop scaling.
    float3x3 w = (float3x3)World;
    float3x3 cofactor = float3x3(cross(w[1], w[2]), cross(w[2], w[0]), cross(w[0], w[1]));
    output.normal = normalize(mul(input.nrm.xyz, cofactor));
    output.color = input.col;
    output.uv = input.tex;
    return output;
}
