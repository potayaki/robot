// Clip-space triangle: no translation, so walking never moves the distant sky.
cbuffer SkyBuffer : register(b7)
{
    float4 RayRight;
    float4 RayUp;
    float4 RayForward;
    float4 TintA;
    float4 TintB;
    float4 SkyParameters;
};

struct SKY_OUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

SKY_OUT main(float3 position : POSITION0)
{
    SKY_OUT output;
    output.position = float4(position.xy, 1, 1);
    output.direction = RayForward.xyz + position.x * RayRight.xyz + position.y * RayUp.xyz;
    return output;
}
