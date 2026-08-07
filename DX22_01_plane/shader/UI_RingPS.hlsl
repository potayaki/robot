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
    float2 center = float2(0.5f, 0.5f);
    float2 offset = In.tex - center;
    float dist = length(offset);
    
    // ==========================================
    // ★ ここで大きさと境界線を微調整できます！
    // ==========================================
    float borderDist = 0.4f; // 赤と青を分ける境界線の距離
    float innerScale = 1.0f; // 青い円（内側）の縮小率（1.0でそのまま、1.2などで小さくなる）
    // ==========================================

    // サンプリング用（画像を読み込む用）のUV座標
    float2 sampleUV = In.tex;
    
    // もし境界線より内側だったら、画像を縮小して読み込む
    if (dist < borderDist)
    {
        sampleUV = center + (offset * innerScale);
    }

    // 縮小済みのUV座標で画像の色を取得
    float4 texColor = g_Texture.Sample(g_SamplerState, sampleUV);
    
    // 背景の黒い部分は透明にして捨てる
    if (texColor.r < 0.05f)
        return float4(0, 0, 0, 0);

    // 角度の計算は実際の描画位置（In.tex）で行う
    float angle = atan2(offset.x, -offset.y);
    if (angle < 0.0f)
        angle += 6.2831853f;
    float angleRate = angle / 6.2831853f;

    float4 outColor = float4(1.0f, 1.0f, 1.0f, texColor.r);
    
    // 色の塗り分けとゲージの削り処理
    if (dist > borderDist)
    {
        // 外側（HP：赤）
        if (angleRate > hpRate)
            outColor.a = 0.0f;
        outColor.rgb = float3(1.0f, 0.05f, 0.1f) * texColor.r;
    }
    else
    {
        // 内側（ミサイル：青）
        if (angleRate > missileRate)
            outColor.a = 0.0f;
        outColor.rgb = float3(0.0f, 0.6f, 1.0f) * texColor.r;
    }

    return outColor * In.col;
}
