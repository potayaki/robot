#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 main(in PS_IN input) : SV_Target
{
    // 粒子ごとの色と残り寿命によるアルファ値を加算合成へ渡す。
    return g_Texture.Sample(g_SamplerState, input.tex) * input.col * material.Diffuse;
}
