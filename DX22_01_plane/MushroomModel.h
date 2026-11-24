/// @file MushroomModel.h
/// @brief キノコモデルの読込と、通常色・予告色の描画。

#pragma once
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "Material.h"
#include <memory>

class MushroomModel {
public:
    /// 使用前の状態と必要な資源を初期化する。
    bool Init();
    /// ワールド座標の足元を基準に、通常サイズへ倍率を掛けて描画する。yawはラジアン。
    void Draw(Camera* camera, DirectX::SimpleMath::Vector3 feet, float yaw, bool red, float scale = 1.0f,
              const DirectX::SimpleMath::Color& tint = DirectX::SimpleMath::Color(1, 1, 1, 0),
              float hitFlash = 0.0f);

private:
    struct Resources;
    /// 群れの全個体で共有するメッシュ・画像・描画シェーダー。
    std::shared_ptr<Resources> m_resources;
};
