/// @file CEmitter.cpp
/// @brief 射撃に合わせた短い閃光と、前方へ散る火花の生成。

#include "CEmitter.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CEmitter::Emit(ParticleManager& particles, const Vector3& position, const Vector3& direction) {
    if (direction.LengthSquared() < 0.0001f) {
        return;
    }
    Vector3 forward = direction;
    forward.Normalize();
    // 真上・真下を狙っても、射撃方向に直交する基底を作る。
    const Vector3 reference = std::abs(forward.y) < 0.99f ? Vector3::UnitY : Vector3::UnitX;
    Vector3 right = reference.Cross(forward);
    right.Normalize();
    const Vector3 up = forward.Cross(right);
    std::uniform_real_distribution<float> random(0.0f, 1.0f);

    // プールの以前の用途に関係なく、描画・移動に必要な状態をすべて設定する。
    CParticle* flash = particles.Spawn();
    flash->SetType(MuzzleFlash);
    flash->SetPosition(position);
    flash->SetRotation(Vector3::Zero);
    flash->SetVelocity(Vector3::Zero);
    flash->SetLife(m_flashLife);
    flash->SetScale(m_flashSize);
    flash->SetColor(Color(1.0f, 0.8f, 0.3f, 1.0f));

    for (int i = 0; i < m_sparkCount; ++i) {
        const float angle = random(m_random) * DirectX::XM_2PI;
        const float radius = std::sqrt(random(m_random)) * m_spread;
        Vector3 velocity = forward + radius * (std::cos(angle) * right + std::sin(angle) * up);
        velocity.Normalize();
        velocity *= m_minSpeed + (m_maxSpeed - m_minSpeed) * random(m_random);

        CParticle* spark = particles.Spawn();
        spark->SetType(MuzzleSpark);
        spark->SetPosition(position);
        spark->SetRotation(Vector3::Zero);
        spark->SetVelocity(velocity);
        spark->SetLife(m_minLife + (m_maxLife - m_minLife) * random(m_random));
        spark->SetScale(m_sparkSize * (0.6f + 0.4f * random(m_random)));
        spark->SetColor(Color(1.0f, 0.4f + 0.35f * random(m_random), 0.08f, 1.0f));
    }
}
