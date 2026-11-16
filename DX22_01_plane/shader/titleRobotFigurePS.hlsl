Texture2D Albedo : register(t0);
Texture2D Metalness : register(t1);
Texture2D Emission : register(t2);
Texture2D Roughness : register(t3);
SamplerState LinearSampler : register(s0);
cbuffer StartLights : register(b8) {
    float sensorCharge;
    float muzzleBoost;
    float2 padding;
};

struct DioramaPixel {
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(DioramaPixel input) : SV_Target {
    float3 n = normalize(input.normal);
    float3 paint = Albedo.Sample(LinearSampler, input.uv).rgb;
    float metal = Metalness.Sample(LinearSampler, input.uv).r;
    float gloss = 1.0 - Roughness.Sample(LinearSampler, input.uv).r;
    float3 lamps = Emission.Sample(LinearSampler, input.uv).rgb;
    const float3 key = normalize(float3(-0.56, 0.92, -0.64));
    const float3 rim = normalize(float3(0.78, 0.58, 0.65));
    float warm = saturate(dot(n, key));
    float cool = saturate(dot(n, rim));
    float3 lighting = float3(0.47, 0.51, 0.57) + warm * float3(0.63, 0.55, 0.47)
                    + cool * float3(0.22, 0.34, 0.46);
    // Broad studio reflections bring out the cast-metal edges as the figure turns.
    float3 edgeLight = pow(cool, 4) * float3(0.12, 0.22, 0.30)
                     + pow(warm, 7) * float3(0.22, 0.19, 0.13);
    float3 reflection = pow(saturate(dot(n, normalize(key + float3(0.52, 0.49, -0.70)))),
                            lerp(12.0, 66.0, gloss)) * metal * (0.04 + 0.22 * gloss);
    float3 muzzleLight = input.color.a * float3(0.60, 0.27, 0.055) * (0.65 + metal * 0.35);
    float3 emission = lamps * float3(0.85, 0.88, 0.65);
    if (sensorCharge > 0) {
        // 発光マスクの縁にも光をにじませ、もともと明るい黄色のセンサーに芯を出す。
        float2 d = float2(0.0025, 0);
        float halo = Emission.Sample(LinearSampler, input.uv + d.xy).r
                   + Emission.Sample(LinearSampler, input.uv - d.xy).r
                   + Emission.Sample(LinearSampler, input.uv + d.yx).r
                   + Emission.Sample(LinearSampler, input.uv - d.yx).r;
        emission += sensorCharge * (lamps.r * float3(1.05, 1.0, 0.88) + halo * float3(0.16, 0.10, 0.018));
    }
    return float4(paint * lighting + metal * edgeLight + reflection + muzzleLight * (1 + muzzleBoost) + emission, 1);
}
