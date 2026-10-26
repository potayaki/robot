#include "common.hlsl"

struct EyePixelInput
{
    float4 position : SV_POSITION;
    float3 localPosition : TEXCOORD0;
    float3 viewPosition : TEXCOORD1;
    float3 viewNormal : TEXCOORD2;
    nointerpolation float3 toLight : TEXCOORD3;
    nointerpolation float3 ambient : TEXCOORD4;
    nointerpolation float3 diffuse : TEXCOORD5;
};

EyePixelInput main(VS_IN input)
{
    EyePixelInput output;
    float4 worldPosition = mul(input.pos, World);
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    output.localPosition = input.pos.xyz;
    output.viewPosition = viewPosition.xyz;
    output.viewNormal = mul(mul(float4(input.nrm.xyz, 0), World), View).xyz;
    // このゲームは光源とViewの定数を頂点シェーダに渡すため、照明情報もここから送る。
    output.toLight = mul(float4(-light.Direction.xyz, 0), View).xyz;
    output.ambient = light.Ambient.rgb;
    output.diffuse = light.Diffuse.rgb;
    return output;
}
