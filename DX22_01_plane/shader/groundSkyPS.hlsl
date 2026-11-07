Texture2D GroundTexture : register(t0);
SamplerState GroundSampler : register(s0);

// Only the distant ground fades into the sky; HUD and nearby enemies stay readable.
cbuffer SkyFogBuffer : register(b8)
{
    float4 FogColorStart;
    float4 FogEnd;
};

float4 main(float4 position : SV_POSITION, float4 color : COLOR0,
            float2 uv : TEXCOORD0, float3 viewPosition : TEXCOORD1) : SV_Target
{
    float3 ground = GroundTexture.Sample(GroundSampler, uv).rgb * color.rgb;
    float fog = smoothstep(FogColorStart.w, FogEnd.x, length(viewPosition));
    return float4(lerp(ground, FogColorStart.rgb, fog), 1);
}
