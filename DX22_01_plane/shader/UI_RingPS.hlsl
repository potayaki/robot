Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer UIBUFFER : register(b6)
{
    float hpRate;
    float missileRate;
    float2 dummy;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT In) : SV_TARGET
{
    float4 texColor = g_Texture.Sample(g_SamplerState, In.uv);
    if (texColor.r < 0.05f)
        return float4(0, 0, 0, 0);

    float2 center = float2(0.5f, 0.5f);
    float2 offset = In.uv - center;
    float dist = length(offset);
    float angle = atan2(offset.x, -offset.y);

    if (angle < 0.0f)
        angle += 6.2831853f;
    float angleRate = angle / 6.2831853f;

    float4 outColor = float4(1.0f, 1.0f, 1.0f, texColor.r);
    
    if (dist > 0.35f)
    {
        if (angleRate > hpRate)
            outColor.a = 0.0f;
        outColor.rgb = float3(1.0f, 0.05f, 0.1f) * texColor.r;
    }
    else
    {
        if (angleRate > missileRate)
            outColor.a = 0.0f;
        outColor.rgb = float3(0.0f, 0.6f, 1.0f) * texColor.r;
    }

    return outColor * In.color;
}
