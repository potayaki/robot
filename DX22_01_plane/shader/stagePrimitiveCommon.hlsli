#include "common.hlsl"

cbuffer PrimitiveBuffer : register(b6)
{
    matrix NormalWorld;
    float4 PrimitiveTint;
    float4 UVTransform;
    uint UsePrimitiveTexture;
    float3 PrimitivePadding;
};

struct PrimitivePixelInput
{
    float4 position : SV_POSITION;
    float3 worldNormal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    nointerpolation float3 toLight : TEXCOORD1;
    nointerpolation float3 ambient : TEXCOORD2;
    nointerpolation float3 diffuse : TEXCOORD3;
};
