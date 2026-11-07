#include "common.hlsl"

cbuffer BoarBones : register(b6) { matrix Bones[64]; }

struct BOAR_IN
{
    float4 position : POSITION0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    uint4 bones : BLENDINDICES0;
    float4 weights : BLENDWEIGHT0;
};

PS_IN main(BOAR_IN input)
{
    // Animals use blended skin weights rather than the mech's rigid one-bone vertices.
    float4 position = 0;
    float3 normal = 0;
    [unroll] for (uint i = 0; i < 4; ++i)
    {
        position += mul(input.position, Bones[input.bones[i]]) * input.weights[i];
        normal += mul(float4(input.normal, 0), Bones[input.bones[i]]).xyz * input.weights[i];
    }
    PS_IN output;
    output.pos = mul(mul(mul(position, World), View), Projection);
    normal = normalize(mul(float4(normal, 0), World).xyz);
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    output.col = float4(input.color.rgb * (diffuse * light.Diffuse.rgb + light.Ambient.rgb), input.color.a);
    output.tex = input.uv;
    return output;
}
