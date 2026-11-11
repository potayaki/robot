/// @file BoarModel.h
/// @brief Boarのモデル、テクスチャ、突進予測線の描画。

#pragma once

#include "BoarAnimationRig.h"
#include "Camera.h"
#include <memory>

// モデル・画像・シェーダーは全Boarで共有し、アニメーションの姿勢だけ個別に持つ。
class BoarModel {
public:
    /// 使用前の状態と必要な資源を初期化する。
    bool Init();

    bool IsReady() const {
        return static_cast<bool>(m_resources);
    }

    /// 経過時間と入力に合わせて状態を更新する。
    void Update(BoarMotion motion, float deltaSeconds, float progress = 0.0f);
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& feet, float yaw, float scale);
    void DrawTelegraph(Camera* camera, DirectX::SimpleMath::Vector3 start, DirectX::SimpleMath::Vector3 end,
                       float groundY, float halfWidth, float progress);

private:
    struct Resources;
    /// 同じ種類の敵で共有する描画資源。
    std::shared_ptr<Resources> m_resources;
    /// モデルの骨格とアニメーションの再生状態。
    BoarAnimationRig m_rig;
    /// 現在再生している動作の種類。
    BoarMotion m_motion = BoarMotion::Idle;
};
