Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

// ★ C++から「1個分のゲージの割合と色」を受け取るように変更
cbuffer UIBUFFER : register(b6)
{
    float rate; // ゲージの残り割合 (0.0 ～ 1.0)
    float3 ringColor; // ゲージの色 (R, G, B)
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(PS_IN In) : SV_TARGET
{
    float4 texColor = g_Texture.Sample(g_SamplerState, In.tex);
    
    // 背景の黒い部分を透明にする
    if (texColor.r < 0.05f)
        return float4(0, 0, 0, 0);

    // 画像の中心からの角度を計算
    float2 center = float2(0.5f, 0.5f);
    float2 offset = In.tex - center;
    float angle = atan2(offset.x, -offset.y);

    if (angle < 0.0f)
        angle += 6.2831853f;
    float angleRate = angle / 6.2831853f;

    // ★ C++から送られた色(ringColor)に、画像の明るさ(texColor.r)を掛ける
    float4 outColor = float4(ringColor * texColor.r, texColor.r);
    
    // ゲージの割合を超えている部分は透明にして削る
    if (angleRate > rate)
        outColor.a = 0.0f;

    return outColor * In.col;
}
