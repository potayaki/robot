/// @file ExperienceOrbRenderer.h
/// @brief 経験値オーブの芯と周囲の光をまとめて描く。

#pragma once
#include "Shader.h"
#include <span>

class Camera;
struct ExperienceOrb;

class ExperienceOrbRenderer {
    /// 複数のオーブの板を一度に転送する動的頂点バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertices;
    /// 芯の不透明度と外側の光の加算を同時に合成する設定。
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blend;
    /// 距離に応じて輪郭を滑らかにする描画プログラム。
    Shader m_shader;

public:
    bool Init();
    void Draw(Camera* camera, std::span<const ExperienceOrb> orbs);
    void Uninit();
};
