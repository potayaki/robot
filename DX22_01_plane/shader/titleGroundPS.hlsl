Texture2D Island : register(t0);
Texture2D Gravel : register(t1);
SamplerState SurfaceSampler : register(s0);
Texture2D<float> ShadowMap : register(t5);
SamplerComparisonState ShadowSampler : register(s1);
cbuffer LightSpace : register(b7) { matrix LightViewProjection; };

struct Input {
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 local : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
};

float keyVisibility(float3 worldPosition) {
    float4 light = mul(float4(worldPosition, 1), LightViewProjection);
    if (light.w < 0.5) return 1;
    float2 uv = light.xy * float2(0.5, -0.5) + 0.5;
    float sum = 0;
    [unroll] for (int y = -2; y <= 2; ++y) {
        [unroll] for (int x = -2; x <= 2; ++x)
            sum += ShadowMap.SampleCmpLevelZero(ShadowSampler, uv + float2(x, y) * (1.6 / 1024), light.z - 0.0012);
    }
    return sum / 25;
}

float4 main(Input input) : SV_Target {
    float3 n = normalize(input.normal);
    float2 p = input.local.xz;
    float3 original = Island.Sample(SurfaceSampler, input.uv).rgb;
    float grit = Gravel.Sample(SurfaceSampler, p * 0.28).r;
    float mottling = Gravel.Sample(SurfaceSampler, p * 0.046 + 0.34).g;
    float3 stone = lerp(float3(0.18, 0.165, 0.14), float3(0.49, 0.46, 0.40),
                        saturate((grit - 0.30) * 1.6)) * (0.80 + 0.28 * mottling);
    // An irregular worn strip runs across the clearing and breaks into grass near the rim.
    float path = abs(p.y - 0.34 * p.x + sin(p.x * 0.59) + (mottling - 0.7) * 9.0);
    float turf = smoothstep(1.2, 6.0, path + 0.16 * length(p)) * 0.82;
    float luma = dot(original, float3(0.30, 0.59, 0.11));
    float3 moss = lerp(luma * float3(0.94, 0.92, 0.76), original, 0.38);
    float3 ground = lerp(stone, moss, turf);
    float3 base = lerp(moss, ground, smoothstep(0.25, 0.78, n.y));
    float key = saturate(dot(n, normalize(float3(-0.6, 1.0, -0.7)))) * keyVisibility(input.worldPosition);
    float fill = saturate(dot(n, normalize(float3(0.8, 0.35, 0.5))));
    return float4(base * (float3(0.40, 0.43, 0.46) + key * float3(0.70, 0.63, 0.52) + fill * 0.19), 1);
}
