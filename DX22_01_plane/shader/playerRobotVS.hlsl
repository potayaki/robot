#include "common.hlsl"

// Each vertex of this rigid mech follows exactly one bone.
cbuffer RobotBones : register(b6)
{
    matrix Bones[128];
}

struct ROBOT_IN
{
    float4 pos : POSITION0;
    float3 nrm : NORMAL0;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
    uint bone : BLENDINDICES0;
};

PS_IN main(ROBOT_IN input)
{
    PS_IN output;
    float4 position = mul(input.pos, Bones[input.bone]);
    output.pos = mul(mul(mul(position, World), View), Projection);
    float3 normal = normalize(mul(mul(float4(input.nrm, 0), Bones[input.bone]), World).xyz);
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    output.col.xyz = input.col.xyz * (diffuse * light.Diffuse.xyz + light.Ambient.xyz) + material.Emission.xyz;
    output.col.a = input.col.a * material.Diffuse.a;
    output.tex = input.tex;
    return output;
}
