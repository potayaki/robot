cbuffer SkyBuffer : register(b7)
{
    float4 RayRight;
    float4 RayUp;
    float4 RayForward;
    float4 TintA;
    float4 TintB;
    float4 SkyParameters; // blend, brightness, rotation A, rotation B
};

Texture2D PreviousSky : register(t0);
Texture2D CurrentSky : register(t1);
SamplerState SkySampler : register(s0);

float3 ReadSky(Texture2D panorama, float3 direction, float rotation, float4 tint)
{
    float s, c;
    sincos(rotation, s, c);
    // Inverse yaw. Source panoramas use +Y up, -Z ahead; this game is left-handed.
    float3 d = float3(c * direction.x - s * direction.z, direction.y,
                     s * direction.x + c * direction.z);
    float2 uv = float2(0.5 - atan2(d.z, d.x) / 6.28318530718,
                       acos(clamp(d.y, -1.0, 1.0)) / 3.14159265359);

    // Correct the longitude derivative at the wrap seam to avoid a blurry vertical line.
    float2 dx = ddx(uv);
    float2 dy = ddy(uv);
    dx.x -= round(dx.x);
    dy.x -= round(dy.x);
    float3 color = panorama.SampleGrad(SkySampler, uv, dx, dy).rgb;
    float gray = dot(color, float3(0.2126, 0.7152, 0.0722));
    return lerp(gray.xxx, color, tint.w) * tint.rgb;
}

float4 main(float4 position : SV_POSITION, float3 direction : TEXCOORD0) : SV_Target
{
    direction = normalize(direction);
    float3 a = ReadSky(PreviousSky, direction, SkyParameters.z, TintA);
    float3 b = ReadSky(CurrentSky, direction, SkyParameters.w, TintB);
    return float4(saturate(lerp(a, b, SkyParameters.x) * SkyParameters.y), 1);
}
