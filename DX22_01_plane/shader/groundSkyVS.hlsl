#include "common.hlsl"

struct GROUND_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 viewPosition : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float3 directColor : COLOR1;
};

GROUND_OUT main(VS_IN input)
{
    GROUND_OUT output;
    float4 worldPosition = mul(input.pos, World);
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    output.viewPosition = viewPosition.xyz;
    output.worldPosition = worldPosition.xyz;
    float3 normal = normalize(mul(float4(input.nrm.xyz, 0), World).xyz);
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    output.directColor = input.col.rgb * diffuse * light.Diffuse.rgb;
    output.color = float4(input.col.rgb * light.Ambient.rgb + output.directColor, input.col.a);
    output.uv = input.tex;
    return output;
}
