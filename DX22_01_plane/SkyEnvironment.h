#pragma once

#include <array>
#include <random>
#include "Camera.h"
#include "SkyCycle.h"
#include "Texture.h"
#include "VertexBuffer.h"

// カメラを中心にした360度の空と、その時間帯に合う照明・遠景の霞を管理する。
class SkyEnvironment
{
public:
    bool Init();
    void Update(float deltaSeconds);
    void Draw(Camera* camera);
    void Uninit();
#ifdef _DEBUG
    void DrawDebugUI();
#endif

private:
    void SetTarget(SkyWeather::Preset preset, bool blend);
    float Random01();

    SkyWeather::Cycle m_cycle;
    SkyWeather::Preset m_previous = SkyWeather::Preset::Day;
    SkyWeather::Preset m_current = SkyWeather::Preset::Day;
    std::mt19937 m_random{std::random_device{}()};
    bool m_automatic = true;
    float m_transitionSeconds = 8.0f;
    float m_rotationDegrees = 0.0f;
    float m_skyBrightness = 1.0f;
    float m_sceneBrightness = 1.0f;
    bool m_ready = false;

    // 朝は夕焼け画像を色調整して使うため、画像は4枚だけでよい。
    std::array<Texture, 4> m_textures{};
    VertexBuffer<VERTEX_3D> m_vertices;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_skyBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_fogBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
};
