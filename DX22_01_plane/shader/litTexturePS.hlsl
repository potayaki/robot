#include"common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 main(in PS_IN input)  : SV_Target
{	
    float4 color;

    if (material.TextureEnable)
    {
    color = g_Texture.Sample(g_SamplerState, input.tex);
    color *= input.col;
        
    }
    else
    {
        color = input.col * material.Diffuse;
    }
    // Sample関数→テクスチャから該当のUV位置のピクセル色を取って来る
    //color = g_Texture.Sample(g_SamplerState, input.tex);
    //color *= input.col;

   // color = input.col;

    return color;
}

