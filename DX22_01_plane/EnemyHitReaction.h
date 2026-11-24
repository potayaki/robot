/// @file EnemyHitReaction.h
/// @brief 敵の当たり判定から独立した、着弾後の描画姿勢。

#pragma once
#include <SimpleMath.h>
#include <algorithm>

struct EnemyHitPose {
    /// 描画する機体だけに加えるワールド座標の水平変位。
    DirectX::SimpleMath::Vector3 offset{};
    /// 着弾直後の発光の強さ（0～1）。
    float flash = 0;
    /// のけぞり・つぶれから元の姿勢へ戻るまでの強さ（0～1）。
    float recoil = 0;
};

class EnemyHitReaction {
    /// 最後の着弾時刻（ゲーム内秒）。負値はまだ被弾していない状態。
    double m_hitTime = -1;
    /// 最後に受けた攻撃の単位方向から上下成分を除いたもの。
    DirectX::SimpleMath::Vector3 m_direction{};

public:
    void Start(double time, DirectX::SimpleMath::Vector3 direction) {
        direction.Normalize();
        direction.y = 0;
        m_direction = direction;
        m_hitTime = time;
    }

    EnemyHitPose Pose(double time, float radius) const {
        constexpr float flashSeconds = 0.075f;
        constexpr float recoilSeconds = 0.16f;
        const float age = static_cast<float>(time - m_hitTime);
        if (m_hitTime < 0 || age < 0 || age >= recoilSeconds) {
            return {};
        }
        EnemyHitPose pose;
        pose.flash = std::clamp(2.0f * (1.0f - age / flashSeconds), 0.0f, 1.0f);
        const float remaining = 1.0f - age / recoilSeconds;
        pose.recoil = remaining * remaining;
        // 大型の敵でも当たり判定から大きくずれない。上から撃ったときも接地位置を保つ。
        pose.offset = m_direction * ((std::min)(4.5f, radius * 0.4f) * pose.recoil);
        return pose;
    }
};
