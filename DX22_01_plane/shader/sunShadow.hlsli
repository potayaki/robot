Texture2D<float> SunShadowMap : register(t5);
SamplerComparisonState SunShadowSampler : register(s1);

cbuffer SunShadowBuffer : register(b11)
{
    matrix LightViewProjection;
    float4 ShadowParameters; // texel size, depth bias, map available, unused
};

float SunVisibility(float3 worldPosition)
{
    if (ShadowParameters.z == 0) return 1;

    float4 clip = mul(float4(worldPosition, 1), LightViewProjection);
    if (clip.w <= 0) return 1;
    float3 projected = clip.xyz / clip.w;
    float2 uv = projected.xy * float2(0.5, -0.5) + 0.5;
    [branch] if (any(uv < 0) || any(uv > 1) || projected.z < 0 || projected.z > 1) return 1;

    float visibility = 0;
    [unroll] for (int y = -1; y <= 1; ++y) {
        [unroll] for (int x = -1; x <= 1; ++x) {
            float2 offset = float2(x, y) * ShadowParameters.x;
            visibility += SunShadowMap.SampleCmpLevelZero(SunShadowSampler, uv + offset,
                                                          projected.z - ShadowParameters.y);
        }
    }
    return visibility / 9;
}
