#include "stagePrimitiveCommon.hlsli"

Texture2D PrimitiveTexture : register(t0);
SamplerState PrimitiveSampler : register(s0);

float4 main(PrimitivePixelInput input, bool frontFace : SV_IsFrontFace) : SV_Target
{
    float4 color = input.color;
    if (UsePrimitiveTexture != 0) color *= PrimitiveTexture.Sample(PrimitiveSampler, input.uv);
    // PNGの透明部分は切り抜く。ステージ部品は不透明描画なので描画順に依存しない。
    clip(color.a - 0.5f);
    float3 normal = normalize(input.worldNormal) * (frontFace ? 1.0f : -1.0f);
    float illumination = saturate(dot(normal, normalize(input.toLight)));
    return float4(color.rgb * (input.ambient + input.diffuse * illumination), 1);
}
