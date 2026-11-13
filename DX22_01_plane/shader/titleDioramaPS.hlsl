Texture2D Surface : register(t0);
SamplerState SurfaceSampler : register(s0);
struct DioramaPixel {
    float4 position : SV_POSITION;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
float4 main(DioramaPixel input, bool front : SV_IsFrontFace) : SV_Target {
    float4 base = Surface.Sample(SurfaceSampler, input.uv) * input.color;
    clip(base.a - 0.45);
    float3 n = normalize(input.normal);
    if (!front) n = -n;
    float key = saturate(dot(n, normalize(float3(-0.6, 1.0, -0.7))));
    float fill = saturate(dot(n, normalize(float3(0.8, 0.35, 0.5))));
    float3 lighting = float3(0.36, 0.39, 0.42) + key * float3(0.73, 0.68, 0.56)
        + fill * float3(0.15, 0.20, 0.25);
    return float4(base.rgb * lighting, 1);
}
