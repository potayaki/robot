Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

cbuffer UIBUFFER : register(b6)
{
    float hpRate;
    float missileRate;
    float2 dummy;
};


struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(PS_IN In) : SV_TARGET
{
    // In.uv ではなく In.tex を使う
    float4 texColor = g_Texture.Sample(g_SamplerState, In.tex);
    
    if (texColor.r < 0.05f)
        return float4(0, 0, 0, 0);

    float2 center = float2(0.5f, 0.5f);
    float2 offset = In.tex - center;
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

    // In.color ではなく In.col を掛ける
    return outColor * In.col;
}
