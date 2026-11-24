cbuffer Glitch : register(b7) {
    float strength;
    float time;
    float2 resolution;
    uint burstIndex;
    float burstPhase;
};
Texture2D scene : register(t7);
SamplerState linearClamp : register(s7);
static const float StreakThicknessScale = 1.30;

struct Pixel {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Pixel VSMain(uint id : SV_VertexID) {
    Pixel p;
    p.uv = float2((id << 1) & 2, id & 2);
    p.position = float4(p.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return p;
}

float noise(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint bits = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return ((bits ^ (bits >> 22u)) >> 8u) * (1.0 / 16777216.0);
}

float3 randomNeonColor(uint seed) {
    float hue = noise(seed + 5u);
    float saturation = lerp(0.72, 1.0, noise(seed + 6u));
    float3 phase = frac(hue + float3(0.0, 2.0 / 3.0, 1.0 / 3.0));
    float3 rgb = saturate(abs(phase * 6.0 - 3.0) - 1.0);
    return lerp(float3(1, 1, 1), rgb, saturation);
}

void applyFineTears(Pixel p, inout float4 color) {
    float clock = time * 12.0;
    float laneHeight = max(5.0, resolution.y / 36.0);
    uint lane = (uint)floor(p.position.y / laneHeight);
    uint seed = (uint)floor(clock) * 4099u + lane * 37u;
    if (noise(seed) >= strength * 0.63) {
        return;
    }

    // The tear narrows and rejoins the original image at both ends.
    float dx = p.uv.x - noise(seed + 1u);
    float reach = lerp(0.15, 0.55, noise(seed + 2u));
    float taper = 1.0 - smoothstep(0.0, reach, abs(dx));
    float baseHalfHeight = max(1.25, laneHeight * lerp(0.14, 0.40, noise(seed + 4u)));
    float halfHeight = min(baseHalfHeight * StreakThicknessScale, 0.5 * laneHeight - 0.5);
    float margin = halfHeight + 0.5;
    float centerY = lane * laneHeight + lerp(margin, laneHeight - margin, noise(seed + 3u));
    float dy = abs(p.position.y - centerY);
    float tear = taper * (1.0 - smoothstep(0.4 * halfHeight * taper, halfHeight * taper + 0.5, dy));
    if (tear <= 0.0) {
        return;
    }

    // Pull a slice sideways and let it retract slightly before the next signal change.
    float direction = noise(seed + 7u) < 0.5 ? -1.0 : 1.0;
    float pull = tear * strength * (1.0 - 0.45 * smoothstep(0.1, 1.0, frac(clock)));
    float2 shift = float2(direction * pull * lerp(0.016, 0.065, noise(seed + 8u)), 0);
    float2 uv = p.uv + shift;
    float2 fringe = shift * 0.18;
    color = scene.SampleLevel(linearClamp, uv, 0);
    float3 leading = scene.SampleLevel(linearClamp, uv + fringe, 0).rgb;
    float3 trailing = scene.SampleLevel(linearClamp, uv - 2.5 * fringe, 0).rgb;

    // Pick out scene contours with colored echoes. On flat surfaces only a fading hairline remains.
    float3 contrast = max(abs(leading - color.rgb), abs(trailing - color.rgb));
    float edge = smoothstep(0.025, 0.30, dot(contrast, float3(0.299, 0.587, 0.114)));
    float tailLength = reach * (dx * direction > 0.0 ? 0.48 : 0.12);
    float filamentHalfHeight = StreakThicknessScale * max(1.1, resolution.y / 670.0);
    float filament = (1.0 - smoothstep(0.0, tailLength, abs(dx)))
                   * (1.0 - smoothstep(0.0, filamentHalfHeight, dy));
    float tint = pull * (0.90 * edge + 0.48 * filament);
    color.rgb = lerp(float3(leading.r, color.g, trailing.b), randomNeonColor(seed), saturate(tint));
}

float burstTravel(float phase) {
    // The image snaps sideways, overshoots slightly on the way back, then settles.
    float kick = 1.0 - smoothstep(0.12, 0.68, phase);
    float recoil = smoothstep(0.42, 0.72, phase) * (1.0 - smoothstep(0.72, 1.0, phase));
    return kick - 0.24 * recoil;
}

void applyBurst(Pixel p, inout float4 base) {
    applyFineTears(p, base);

    // Fade in the wide tear, then extend its displacement and RGB split up to 200% intensity.
    float critical = smoothstep(0.5, 1.0, strength);
    if (critical <= 0.0) {
        return;
    }

    // One longer rupture holds its position for the burst while the fine interference keeps moving.
    uint seed = burstIndex * 167u;
    float dx = p.uv.x - lerp(0.35, 0.65, noise(seed + 1u));
    float taper = 1.0 - smoothstep(0.10, 0.60, abs(dx));
    float centerY = lerp(0.30, 0.70, noise(seed + 2u)) + dx * lerp(-0.020, 0.020, noise(seed + 4u));
    float halfHeight = lerp(0.012, 0.046, strength) * lerp(0.50, 1.0, taper);
    float y = (p.uv.y - centerY) / halfHeight;
    float body = 1.0 - smoothstep(0.52, 1.0, abs(y));
    float seam = 1.0 - smoothstep(0.08, 0.22, abs(y - 1.24));
    float coverage = taper * max(body, seam);
    if (coverage <= 0.0) {
        return;
    }

    // The narrow seam slips in the opposite direction, separating the two sides of the tear.
    float direction = noise(seed + 3u) < 0.5 ? -1.0 : 1.0;
    float shear = seam > 0.0 ? -0.42 : 1.0;
    float distance = direction * shear * taper * strength * saturate(strength)
                   * lerp(0.065, 0.115, noise(seed + 7u));
    float travel = burstTravel(burstPhase);
    float2 shiftedUv = p.uv + float2(distance * travel, 0);
    float split = critical * max(1.0, strength);
    float2 rgbOffset = float2(direction * shear * split * taper * abs(travel) * 0.009, 0);
    float4 displaced = scene.SampleLevel(linearClamp, shiftedUv, 0);
    displaced.r = scene.SampleLevel(linearClamp, shiftedUv + rgbOffset, 0).r;
    displaced.b = scene.SampleLevel(linearClamp, shiftedUv - rgbOffset, 0).b;

    // Sample the same contour at a delayed position so a colored copy lingers during recovery.
    float delayed = smoothstep(0.0, 0.19, burstPhase) * burstTravel(saturate(burstPhase - 0.19));
    float2 echoUv = p.uv + float2(distance * delayed, 0);
    float3 echo = scene.SampleLevel(linearClamp, echoUv, 0).rgb;
    float2 edgeStep = float2(direction * max(1.0 / resolution.x, 0.0035 * strength), 0);
    float3 neighbor = scene.SampleLevel(linearClamp, echoUv + edgeStep, 0).rgb;
    const float3 luma = float3(0.299, 0.587, 0.114);
    float separation = smoothstep(0.02, 0.35, dot(abs(echo - displaced.rgb), luma));
    float edge = smoothstep(0.025, 0.32, dot(abs(neighbor - echo), luma));
    float3 neon = randomNeonColor(seed);
    float3 echoColor = lerp(echo, seam > 0.0 ? neon.brg : neon, 0.78);
    displaced.rgb = lerp(displaced.rgb, echoColor, saturate(strength * (0.30 * separation + 0.67 * edge)));

    float release = 1.0 - smoothstep(0.70, 1.0, burstPhase);
    base = lerp(base, displaced, coverage * critical * release);
}

float4 PSMain(Pixel p) : SV_Target {
    float4 color = scene.SampleLevel(linearClamp, p.uv, 0);
    if (burstPhase >= 0.0 && burstPhase < 1.0) {
        applyBurst(p, color);
    }

    // Screen-space interference persists between tears. The game clock freezes the grain on pause.
    uint2 pixel = (uint2)p.position.xy;
    uint frame = (uint)floor(time * 20.0);
    float grain = noise(pixel.x + pixel.y * 65537u + frame * 2654435761u) - 0.5;
    float lineSpacing = max(3.0, round(resolution.y / 270.0));
    float scanline = 0.5 + 0.5 * cos(6.2831853 * p.position.y / lineSpacing);
    color.rgb = saturate(color.rgb * (1.0 - 0.065 * strength * scanline) + 0.068 * strength * grain);
    return color;
}
