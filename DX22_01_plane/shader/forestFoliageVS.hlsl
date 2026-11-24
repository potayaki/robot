#include "common.hlsl"
#include "meadowViewDistance.hlsli"
#include "meadowWind.hlsli"

struct FoliageVertex
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 viewPosition : TEXCOORD1;
};

FoliageVertex main(VS_IN input)
{
    float4 worldPoint = mul(input.pos, World);
    float4 rootWorld = mul(float4(0, 0, 0, 1), World);
    float2 root = rootWorld.xz;
    float height = length(mul(float4(0, 1, 0, 0), World).xyz);
    float t = saturate(input.pos.y); // ForestModel normalizes meshes to unit height.
    float phase = MeadowPhase();
    float gust = MeadowGust(root, phase);
    float seed = frac(dot(root, float2(0.017, 0.031)));
    float bend = t * t * height;
    worldPoint.xz += MeadowLean(root, phase, gust, seed) * (0.42 * bend);
    worldPoint.y -= gust * 0.024 * bend;

    // Shrink each whole clump around its planted root as it approaches the draw limit.
    float rootDistance = length(mul(rootWorld, View).xyz);
    float fade = 1 - smoothstep(GroundCoverFadeStart, GroundCoverFadeEnd, rootDistance);
    worldPoint.xyz = lerp(rootWorld.xyz, worldPoint.xyz, fade);
    float4 viewPoint = mul(worldPoint, View);
    float3 normal = normalize(mul(float4(input.nrm.xyz, 0), World).xyz);
    float sun = saturate(-dot(light.Direction.xyz, normal));
    FoliageVertex output;
    output.position = mul(viewPoint, Projection);
    output.viewPosition = viewPoint.xyz;
    output.color = float4(input.col.rgb * (light.Ambient.rgb + sun * light.Diffuse.rgb), input.col.a);
    output.uv = input.tex;
    return output;
}
