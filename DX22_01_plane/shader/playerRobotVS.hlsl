#include "common.hlsl"

// Each vertex of this rigid mech follows exactly one bone.
cbuffer RobotBones : register(b6)
{
    matrix Bones[128];
    float4 MuzzleLights[2];
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
    float4 worldPosition = mul(position, World);
    output.pos = mul(mul(worldPosition, View), Projection);
    float3 normal = normalize(mul(mul(float4(input.nrm, 0), Bones[input.bone]), World).xyz);
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    output.col.xyz = input.col.xyz * (diffuse * light.Diffuse.xyz + light.Ambient.xyz) + material.Emission.xyz;
    // Light nearby barrel and armor faces for the instant the gun fires.
    [unroll] for (int side = 0; side < 2; ++side)
    {
        if (MuzzleLights[side].w > 0)
        {
            float3 toFlash = MuzzleLights[side].xyz - worldPosition.xyz;
            float distanceSq = dot(toFlash, toFlash);
            float falloff = saturate(1 - distanceSq / (13.0 * 13.0));
            float facing = saturate(dot(normal, toFlash * rsqrt(max(distanceSq, 0.001))));
            output.col.rgb += float3(2.65, 1.15, 0.36) * MuzzleLights[side].w
                            * falloff * falloff * (0.22 + 0.78 * facing);
        }
    }
    output.col.a = input.col.a * material.Diffuse.a;
    output.tex = input.tex;
    return output;
}
