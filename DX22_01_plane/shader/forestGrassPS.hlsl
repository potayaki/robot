float4 main(float4 position : SV_POSITION, float3 color : COLOR0) : SV_Target
{
    return float4(color, 1);
}
