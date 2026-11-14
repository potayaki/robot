#include "stagePrimitiveCommon.hlsli"

PrimitivePixelInput main(VS_IN input)
{
    PrimitivePixelInput output;
    output.position = mul(mul(mul(input.pos, World), View), Projection);
    output.worldNormal = mul(float4(input.nrm.xyz, 0), NormalWorld).xyz;
    output.color = input.col * PrimitiveTint;
    output.uv = input.tex * UVTransform.xy + UVTransform.zw;
    output.toLight = -light.Direction.xyz;
    output.ambient = light.Enable ? light.Ambient.rgb : float3(1, 1, 1);
    output.diffuse = light.Enable ? light.Diffuse.rgb : float3(0, 0, 0);
    return output;
}
