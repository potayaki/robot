/*
cbuffer Glitch : register(b7) { float strength; float time; float2 resolution; };
Texture2D scene : register(t7);
SamplerState linearClamp : register(s7);
struct Pixel { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
Pixel VSMain(uint id : SV_VertexID)
{
    Pixel p;
    p.uv = float2((id << 1) & 2, id & 2);
    p.position = float4(p.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return p;
}
float hash(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
float4 PSMain(Pixel p) : SV_Target
{
    float tick = floor(time * lerp(8, 28, strength));
    float band = floor(p.uv.y * 72);
    float random = hash(float2(band, tick));
    float tear = step(1 - strength * 0.28, random);
    float offset = (hash(float2(tick, band + 19)) - 0.5) * strength * 0.065 * tear;
    float2 uv = p.uv + float2(offset, 0);
    float3 color = scene.Sample(linearClamp, uv).rgb;
    float split = strength * (0.0015 + tear * 0.009);
    color.r = scene.Sample(linearClamp, uv + float2(split, 0)).r;
    float grain = hash(floor(p.uv * resolution / 2) + tick * 17);
    float blocks = step(0.96 - strength * 0.1, hash(floor(p.uv * float2(34, 65)) + tick));
    float edge = smoothstep(0.22, 0.72, length(p.uv - 0.5));
    float scanline = 0.5 + 0.5 * sin(p.position.y * 1.57);
    float redNoise = strength * (grain * 0.12 + blocks * 0.13 + tear * 0.16 + edge * 0.25);
    color *= 1 - strength * (edge * 0.22 + scanline * 0.06);
    color = lerp(color, float3(1, 0.015, 0.035), saturate(redNoise));
    return float4(saturate(color), 1);
}

*/

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer ConstantBuffer : register(b0) {
    float g_time; // 経過時間
    float g_intensity; // グリッチの強度
    float2 g_screenSize; // 画面サイズ
}

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// 乱数生成用の簡易的なハッシュ関数
float random(float2 st) {
    return frac(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

float4 main(PS_INPUT input) : SV_Target {
    float2 uv = input.uv;

    // 1. 時間とY座標を元にしたブロック状のノイズ
    float blockNoise = step(0.95, random(float2(floor(uv.y * 20.0), floor(g_time * 10.0)))) * g_intensity;

    // 2. ブロックノイズが発生した部分のX方向のUVを大きくズラす
    uv.x += (random(float2(g_time, uv.y)) - 0.5) * blockNoise * 0.2;

    // 3. RGBの色ズレ（色収差）を起こす
    float r = g_texture.Sample(g_sampler, uv + float2(g_intensity * 0.02, 0.0)).r;
    float g = g_texture.Sample(g_sampler, uv).g;
    float b = g_texture.Sample(g_sampler, uv - float2(g_intensity * 0.02, 0.0)).b;

    // 4. スキャンライン（横線）の追加
    float scanline = sin(uv.y * g_screenSize.y * 1.5) * 0.04 * g_intensity;

    float3 color = float3(r, g, b) - scanline;

    return float4(color, 1.0);
}
