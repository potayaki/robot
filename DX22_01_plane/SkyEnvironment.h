/// @file SkyEnvironment.h
/// @brief 空画像、時間帯の補間、照明と遠景の霧。

#pragma once

#include <array>
#include <random>
#include "Camera.h"
#include "SkyCycle.h"
#include "Texture.h"
#include "VertexBuffer.h"

// カメラを中心にした360度の空と、その時間帯に合う照明・遠景の霞を管理する。
class SkyEnvironment {
public:
    /// 使用前の状態と必要な資源を初期化する。
    bool Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(float deltaSeconds);
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera);
    /// 使用した資源を解放して終了する。
    void Uninit();
#ifdef _DEBUG
    void DrawDebugUI();
#endif

private:
    void SetTarget(SkyWeather::Preset preset, bool blend);
    float Random01();

    /// 時刻の進行と朝・昼の曇り抽選を管理する状態。
    SkyWeather::Cycle m_cycle;
    /// 切り替え前の空のプリセット。
    SkyWeather::Preset m_previous = SkyWeather::Preset::Day;
    /// 切り替え先の空のプリセット。
    SkyWeather::Preset m_current = SkyWeather::Preset::Day;
    /// 抽選を行う乱数生成器。
    std::mt19937 m_random{std::random_device{}()};
    /// 時間経過で空と天候を自動的に切り替えるか。
    bool m_automatic = true;
    /// 空のプリセットを補間する進行時間（秒）。
    float m_transitionSeconds = 8.0f;
    /// 空の模様を水平に回す角度（度）。
    float m_rotationDegrees = 0.0f;
    /// 空の描画に掛ける明るさの倍率。
    float m_skyBrightness = 1.0f;
    /// 地形とモデルに掛ける明るさの倍率。
    float m_sceneBrightness = 1.0f;
    /// 必要な描画資源の準備が完了しているか。
    bool m_ready = false;

    // 朝は夕焼け画像を色調整して使うため、画像は4枚だけでよい。
    /// 描画に使用するテクスチャ一覧。
    std::array<Texture, 4> m_textures{};
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertices;
    /// 頂点の座標変換を担当するGPUシェーダー。
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    /// 描画ピクセルの色を計算するGPUシェーダー。
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    /// 頂点構造とシェーダーの入力項目の対応。
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    /// 空の向き・色・補間率をGPUへ渡す定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_skyBuffer;
    /// 遠景の霧の色と距離をGPUへ渡す定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_fogBuffer;
    /// テクスチャを拡大縮小するときの参照方法。
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    /// 深度テストと深度書き込みの設定。
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth;
    /// カリングや塗りつぶし方法を指定する描画設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
};
