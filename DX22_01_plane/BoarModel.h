#pragma once

#include "BoarAnimationRig.h"
#include "Camera.h"
#include <memory>

// モデル・画像・シェーダーは全Boarで共有し、アニメーションの姿勢だけ個別に持つ。
class BoarModel
{
public:
    bool Init();
    bool IsReady() const { return static_cast<bool>(m_resources); }
    void Update(BoarMotion motion, float deltaSeconds, float progress = 0.0f);
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& feet, float yaw, float scale);
    void DrawTelegraph(Camera* camera, DirectX::SimpleMath::Vector3 start,
                       DirectX::SimpleMath::Vector3 end, float groundY, float halfWidth, float progress);

private:
    struct Resources;
    std::shared_ptr<Resources> m_resources;
    BoarAnimationRig m_rig;
    BoarMotion m_motion = BoarMotion::Idle;
};
