#include "common.hlsl"
#include "meadowViewDistance.hlsli"
#include "meadowWind.hlsli"

struct BladeInstance
{
    float4 root : GRASS_ROOT;     // world XYZ, seed [0,1]
    float4 traits : GRASS_TRAITS; // height, half-width, yaw, rest tip offset
};

struct BladeVertex
{
    float4 position : SV_POSITION;
    float3 ambientColor : COLOR0;
    float3 directColor : COLOR1;
    float3 worldPosition : TEXCOORD0;
};

BladeVertex main(BladeInstance blade, uint vertexID : SV_VertexID)
{
    // Base and shoulder pairs form a quad; one more triangle tapers to a curved tip.
    static const float2 ribbon[9] = {
        float2(-1, 0), float2(-0.60, 0.56), float2(1, 0),
        float2(1, 0), float2(-0.60, 0.56), float2(0.60, 0.56),
        float2(-0.60, 0.56), float2(0, 1), float2(0.60, 0.56)
    };
    float2 vertex = ribbon[vertexID];
    float t = vertex.y;
    float seed = blade.root.w;
    float height = blade.traits.x;
    float phase = MeadowPhase();
    float gust = MeadowGust(blade.root.xz, phase);
    float2 lean = MeadowLean(blade.root.xz, phase, gust, seed);
    float2 restDirection = float2(cos(blade.traits.z), sin(blade.traits.z));

    float3 rootView = mul(float4(blade.root.xyz, 1), View).xyz;
    float fade = 1 - smoothstep(GrassBladeFadeStart, GrassBladeFadeEnd, length(rootView));
    // Retain a readable ribbon width when viewed across the original blade direction.
    float2 right = normalize(float2(View._11, View._31));
    float2 widthAxis = right + float2(-right.y, right.x) * (0.28 * sin(blade.traits.z));
    float3 bladePoint = blade.root.xyz;
    bladePoint.xz += (widthAxis * blade.traits.y * vertex.x +
                 (restDirection * blade.traits.w + lean * height) * (t * t)) * fade;
    bladePoint.y += height * (t - (0.025 + 0.10 * gust) * t * t) * fade;

    float3 base = lerp(float3(0.27, 0.42, 0.18), float3(0.38, 0.51, 0.22), seed);
    float3 tip = lerp(float3(0.54, 0.72, 0.32), float3(0.73, 0.80, 0.41), seed);
    float3 normal = normalize(float3(-restDirection.x * 0.22, 1, -restDirection.y * 0.22));
    float diffuse = saturate(-dot(light.Direction.xyz, normal));
    float3 bladeColor = lerp(base, tip, t) * (1 + gust * t * 0.08);
    BladeVertex output;
    output.position = mul(mul(float4(bladePoint, 1), View), Projection);
    output.ambientColor = bladeColor * light.Ambient.rgb;
    output.directColor = bladeColor * diffuse * light.Diffuse.rgb;
    output.worldPosition = bladePoint;
    return output;
}
