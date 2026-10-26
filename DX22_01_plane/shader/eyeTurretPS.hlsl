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

float4 main(EyePixelInput input) : SV_Target
{
    // ローカル+Zが瞳の正面。球を回すだけで、虹彩と瞳もビーム方向を向く。
    float3 localNormal = normalize(input.localPosition);
    float radius = length(localNormal.xy) / 0.54;
    float angle = atan2(localNormal.y, localNormal.x);
    float edgeWidth = max(fwidth(radius), 0.008);
    float front = step(0.0, localNormal.z);
    float irisMask = front * (1.0 - smoothstep(1.0 - edgeWidth, 1.0 + edgeWidth, radius));

    // 白目はわずかに暖色を含め、虹彩の外側にだけ薄い血管を加える。
    float veinWave = abs(sin(angle * 17.0 + sin(radius * 9.0) * 0.8));
    float veinWidth = max(fwidth(veinWave), 0.025);
    float vein = (1.0 - smoothstep(0.025, 0.025 + veinWidth, veinWave));
    vein *= smoothstep(1.1, 1.7, radius) * 0.12;
    float3 sclera = lerp(float3(0.91, 0.88, 0.83), float3(0.64, 0.26, 0.22), vein);

    // 茶色の虹彩に放射状の細い繊維を入れる。遠景では細部を弱めてちらつきを抑える。
    float fiberPhase = angle * 79.0 + sin(angle * 23.0) * 2.0 + radius * 14.0;
    float fiberVisibility = 1.0 - smoothstep(0.4, 2.5, fwidth(fiberPhase));
    float fibers = 0.5 + 0.5 * sin(fiberPhase) * fiberVisibility;
    float3 iris = lerp(float3(0.15, 0.075, 0.032), float3(0.46, 0.29, 0.11), fibers);
    iris *= 0.75 + 0.25 * sin(saturate(radius) * 3.14159);
    iris = lerp(iris, float3(0.055, 0.038, 0.023), smoothstep(0.85, 1.0, radius));

    // 人の目と同じ丸い黒い瞳。境界はピクセル幅に合わせて滑らかにする。
    float pupil = 1.0 - smoothstep(0.34 - edgeWidth, 0.34 + edgeWidth, radius);
    iris = lerp(iris, float3(0.006, 0.008, 0.009), pupil);
    float3 baseColor = lerp(sclera, iris, irisMask);

    // ピクセルごとの陰影と角膜の光沢で、面の境目が見えない立体的な眼球にする。
    float3 normal = normalize(input.viewNormal);
    float3 toCamera = normalize(-input.viewPosition);
    float3 toLight = normalize(input.toLight);
    float diffuse = saturate(dot(normal, toLight));
    float3 lighting = max(input.ambient, float3(0.24, 0.24, 0.25)) + input.diffuse * diffuse;
    float3 halfVector = normalize(toCamera + toLight);
    float gloss = pow(saturate(dot(normal, halfVector)), 96.0);
    float3 color = baseColor * lighting + gloss * lerp(0.32, 0.8, irisMask);
    return float4(saturate(color), 1.0);
}
