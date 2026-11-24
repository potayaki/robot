#include "sunShadow.hlsli"

float4 main(float4 position : SV_POSITION, float3 ambientColor : COLOR0,
            float3 directColor : COLOR1, float3 worldPosition : TEXCOORD0) : SV_Target
{
    return float4(ambientColor + directColor * SunVisibility(worldPosition), 1);
}
