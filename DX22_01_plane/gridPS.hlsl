struct VS_OUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

// ====================================================================
// Pristine Grid 処理
// ====================================================================
float PristineGrid(float2 uv, float lineWidth) {
    float2 ddx_uv = ddx(uv);
    float2 ddy_uv = ddy(uv);
    float2 uvDeriv = float2(length(float2(ddx_uv.x, ddy_uv.x)), length(float2(ddx_uv.y, ddy_uv.y)));

    float2 lineWidth2 = float2(lineWidth, lineWidth);
    float2 invertLine = step(0.5, lineWidth2);
    float2 targetWidth = lerp(lineWidth2, 1.0 - lineWidth2, invertLine);

    float2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
    float2 lineAA = uvDeriv * 1.5;

    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = lerp(1.0 - gridUV, gridUV, invertLine);

    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = lerp(grid2, 1.0 - grid2, invertLine);

    return lerp(grid2.x, 1.0, grid2.y);
}

// ====================================================================
// メイン処理
// ====================================================================
float4 main(VS_OUT input) : SV_TARGET {
    // 20.0f を変えるとマスの大きさが変わります
    float2 gridUV = input.worldPos.xz / 20.0f;
    
    // グリッドの値を計算（線の太さは 3% = 0.03）
    float gridValue = PristineGrid(gridUV, 0.03f);

    // 色の設定（サイバー空間風の青と黒）
    float4 lineColor = float4(0.0f, 0.8f, 1.0f, 1.0f); // 水色
    float4 bgColor = float4(0.05f, 0.05f, 0.1f, 1.0f); // 紺色

    // 背景色と線の色を合成して出力
    return lerp(bgColor, lineColor, gridValue);
}
