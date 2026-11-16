Texture2D Surface : register(t0);
SamplerState SurfaceSampler : register(s0);
struct DioramaPixel {
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
#ifdef TITLE_ENEMY_PORTRAIT
cbuffer PortraitLight : register(b9) {
    float4 eyeAndWeight;
    float4 fade;
};
#endif
float4 main(DioramaPixel input, bool front : SV_IsFrontFace) : SV_Target {
    float4 base = Surface.Sample(SurfaceSampler, input.uv) * input.color;
    clip(base.a - 0.45);
    float3 n = normalize(input.normal);
    if (!front) n = -n;
    float key = saturate(dot(n, normalize(float3(-0.6, 1.0, -0.7))));
    float fill = saturate(dot(n, normalize(float3(0.8, 0.35, 0.5))));
    float3 lighting = float3(0.36, 0.39, 0.42) + key * float3(0.73, 0.68, 0.56)
        + fill * float3(0.15, 0.20, 0.25);
#ifdef TITLE_ENEMY_PORTRAIT
    // 寄りの途中で別の敵が顔を横切る時も、深度と輪郭を保って消えていく。
    if (fade.x > 0) {
        const float threshold[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
        uint2 p = (uint2)input.position.xy & 3;
        clip((threshold[p.y * 4 + p.x] + 0.5) / 16.0 - fade.x);
    }
    float3 eye = eyeAndWeight.xyz;
    float3 right = normalize(cross(float3(0,1,0), -eye));
    float3 up = cross(-eye, right);
    float softbox = saturate(dot(n, normalize(eye * 0.86 + up * 0.75 - right * 0.52)));
    float rim = saturate(dot(n, normalize(right * 0.85 + up * 0.35 - eye * 0.40)));
    float3 studio = float3(0.64, 0.66, 0.72) + softbox * float3(0.67, 0.62, 0.55)
        + rim * float3(0.19, 0.24, 0.29);
    lighting = lerp(lighting, studio, eyeAndWeight.w);
#endif
    return float4(base.rgb * lighting, 1);
}
