cbuffer MeadowWindBuffer : register(b10)
{
    float4 WindClock;
};

static const float2 WindAlong = float2(0.894427, 0.447214);
static const float2 WindAcross = float2(-0.447214, 0.894427);

// All temporal harmonics are integers, so the 64-second clock wraps continuously.
float MeadowPhase()
{
    return frac(WindClock.x / 64.0) * 6.2831853;
}

float MeadowGust(float2 worldXZ, float phase)
{
    float along = dot(worldXZ, WindAlong);
    float across = dot(worldXZ, WindAcross);
    float front = sin(along * 0.016 - 9 * phase + 0.95 * sin(across * 0.012 + 2 * phase));
    float wake = sin(along * 0.027 - 13 * phase + 0.70 * sin(across * 0.019 - 3 * phase));
    return smoothstep(-0.85, 0.85, front * 0.73 + wake * 0.27);
}

float2 MeadowLean(float2 worldXZ, float phase, float gust, float seed)
{
    float flutter = sin(dot(worldXZ, float2(0.071, 0.046)) - 23 * phase + seed * 6.2831853);
    return WindAlong * (0.09 + 0.47 * gust) + WindAcross * (0.04 + 0.09 * gust) * flutter;
}
