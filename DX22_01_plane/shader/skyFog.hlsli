cbuffer SkyFogBuffer : register(b8)
{
    float4 FogColorStart;
    float4 FogEnd;
};

float3 ApplySkyFog(float3 color, float3 viewPosition, float rangeOffset)
{
    // The sky renderer leaves b8 unbound if the environment failed to load.
    if (FogEnd.x <= FogColorStart.w) return color;
    float fog = smoothstep(FogColorStart.w, FogEnd.x, length(viewPosition) - rangeOffset);
    return lerp(color, FogColorStart.rgb, fog);
}
