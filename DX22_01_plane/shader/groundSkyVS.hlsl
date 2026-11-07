#include "common.hlsl"

struct GROUND_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 viewPosition : TEXCOORD1;
};

GROUND_OUT main(VS_IN input)
{
    GROUND_OUT output;
    float4 viewPosition = mul(mul(input.pos, World), View);
    output.position = mul(viewPosition, Projection);
    output.viewPosition = viewPosition.xyz;
    float3 normal = normalize(mul(float4(input.nrm.xyz, 0), World).xyz);
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    output.color = float4(input.col.rgb * (diffuse * light.Diffuse.rgb + light.Ambient.rgb), input.col.a);
    output.uv = input.tex;
    return output;
}
