/// @brief 経験値の粒の落下、地上での待機、プレイヤーへの吸引。
#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <cmath>

struct ExperienceOrb {
    /// 地形の起伏やジャンプにかかわらず、水平距離で吸引を開始する。
    static constexpr float MagnetRadius = 65.0f;
    /// 地面で待つときの粒の中心の高さ（ワールド単位）。
    static constexpr float HoverHeight = 7.0f;
    /// 描画と吸収判定で共有するワールド座標。
    DirectX::SimpleMath::Vector3 position;
    /// 光の脈動と浮遊の位相（ラジアン）。
    float phase;
    /// 吸収時にプレイヤーへ渡す基礎経験値。
    int experience;

    ExperienceOrb(DirectX::SimpleMath::Vector3 origin, float groundY, int value, float initialPhase,
                  const DirectX::SimpleMath::Vector3& launchVelocity)
        : position(origin), phase(initialPhase), experience(value), m_dropVelocity(launchVelocity) {
        position.y = (std::max)(origin.y, groundY + HoverHeight);
    }

    bool IsAttracting() const {
        return m_motion == Motion::Attracting;
    }

    /// 粒が機体に届いたステップだけtrueを返す。呼び出し元は届いた粒を取り除く。
    template <class HeightAt>
    bool Advance(float seconds, const DirectX::SimpleMath::Vector3& receiver, const HeightAt& heightAt) {
        if (seconds <= 0.0f) {
            return false;
        }
        constexpr float pulseRate = 3.6f;
        phase = std::fmod(phase + pulseRate * seconds, DirectX::XM_2PI);
        if (m_motion == Motion::Falling) {
            // 水平方向の速度は指数減衰。高い敵から落ちても到達距離は水平初速 / drag に収まる。
            constexpr float drag = 2.8f;
            const float decay = std::exp(-drag * seconds);
            const float travelSeconds = (1.0f - decay) / drag;
            position.x += m_dropVelocity.x * travelSeconds;
            position.z += m_dropVelocity.z * travelSeconds;
            m_dropVelocity.x *= decay;
            m_dropVelocity.z *= decay;

            constexpr float gravity = 180.0f;
            position.y += (m_dropVelocity.y - 0.5f * gravity * seconds) * seconds;
            m_dropVelocity.y -= gravity * seconds;
            const float restY = heightAt(position.x, position.z) + HoverHeight;
            if (position.y <= restY) {
                position.y = restY;
                m_motion = Motion::Waiting;
            }
            return false;
        }

        const auto offset = receiver - position;
        if (m_motion == Motion::Waiting) {
            if (offset.x * offset.x + offset.z * offset.z > MagnetRadius * MagnetRadius) {
                return false;
            }
            m_motion = Motion::Attracting;
        }
        // いったん動いた粒は、プレイヤーが走ったりジャンプしたりしても後を追う。
        constexpr float acceleration = 460.0f;
        constexpr float maxSpeed = 300.0f;
        constexpr float contactRadius = 3.0f;
        const float nextSpeed = (std::min)(m_pullSpeed + acceleration * seconds, maxSpeed);
        const float averageSpeed = (m_pullSpeed + nextSpeed) * 0.5f;
        const float travel = averageSpeed * seconds;
        m_pullSpeed = nextSpeed;
        const float distance = offset.Length();
        if (distance <= contactRadius + travel) {
            position = receiver;
            return true;
        }
        position += offset * (travel / distance);
        // 追い始めだけ浮かせ、草や自機の脚の陰から弧を描いて胴体へ入る。
        constexpr float liftSeconds = 0.22f;
        constexpr float initialLiftSpeed = 130.0f;
        const float lift =
            (std::max)(0.0f, 1.0f - (averageSpeed - InitialPullSpeed) / (acceleration * liftSeconds));
        position.y += initialLiftSpeed * lift * seconds;
        return false;
    }

private:
    enum class Motion {
        Falling,
        Waiting,
        Attracting
    };
    /// 着地後に吸引を受け付ける。追尾を始めた粒は待機へは戻らない。
    Motion m_motion = Motion::Falling;
    /// 落下中のワールド速度。左右の散りは減衰し、上下には重力が働く（ワールド単位/秒）。
    DirectX::SimpleMath::Vector3 m_dropVelocity;
    /// 吸引を始めるときの水平・上下共通の追尾速度（ワールド単位/秒）。
    static constexpr float InitialPullSpeed = 58.0f;
    /// 吸引開始から徐々に速くなる追尾速度（ワールド単位/秒）。
    float m_pullSpeed = InitialPullSpeed;
};
