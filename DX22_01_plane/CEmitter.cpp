/// @file CEmitter.cpp
/// @brief 銃口の噴炎、衝撃環と破片、撃破の光の生成。

#include "CEmitter.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    /// プール内の前回の用途に関係なく、粒子の姿勢・運動・寿命を初期化する。
    void EmitParticle(ParticleManager& particles, ParticleType type, const Vector3& position,
                      const Vector3& velocity, float life, const Vector3& size, const Color& color) {
        CParticle* particle = particles.Spawn();
        particle->SetType(type);
        particle->SetPosition(position);
        particle->SetRotation(Vector3::Zero);
        particle->SetVelocity(velocity);
        particle->SetLife(life);
        particle->SetScale(size);
        particle->SetColor(color);
    }
} // namespace

void CEmitter::Emit(ParticleManager& particles, const Vector3& position, const Vector3& direction) {
    if (direction.LengthSquared() < 0.0001f) {
        return;
    }
    Vector3 forward = direction;
    forward.Normalize();
    // 左右交互の連射でも同じ銃口が次に光る前に消える。30fpsでも先端と芯が読める長さ。
    constexpr float plumeSeconds = 0.065f;
    std::uniform_real_distribution<float> length(9.0f, 13.0f);
    std::uniform_real_distribution<float> width(3.0f, 4.3f);
    const float plumeLength = length(m_random);
    const float plumeWidth = width(m_random);
    EmitParticle(particles, MuzzlePlume, position, forward, plumeSeconds, Vector3(plumeLength, plumeWidth, 1),
                 Color(1.0f, 0.40f, 0.07f, 1.0f));
}

void CEmitter::EmitImpact(ParticleManager& particles, const Vector3& position, const Vector3& incoming) {
    if (incoming.LengthSquared() < 0.0001f) {
        return;
    }
    // 縦に撃つ場合も入射方向の手前側へ散らす。
    Vector3 back = -incoming;
    back.Normalize();
    Vector3 right = (std::abs(back.y) < 0.99f ? Vector3::UnitY : Vector3::UnitX).Cross(back);
    right.Normalize();
    const Vector3 up = back.Cross(right);
    constexpr int chipCount = 5;
    constexpr float ringDiameter = 15.0f;
    EmitParticle(particles, ImpactRing, position, Vector3::Zero, 0.095f,
                 Vector3(ringDiameter, ringDiameter, 1), Color(0.72f, 0.015f, 0.22f, 1.0f));

    // ドロップや武器抽選の乱数を消費せず、同じ場所への連射でも破片の角度と大きさを変える。
    static thread_local std::minstd_rand impactRandom{7301};
    std::uniform_real_distribution<float> sample(0.0f, 1.0f);
    const float phase = DirectX::XM_2PI * sample(impactRandom);
    for (int i = 0; i < chipCount; ++i) {
        const float angle = phase + DirectX::XM_2PI * (i + 0.55f * sample(impactRandom)) / chipCount;
        const Vector3 sideways = std::cos(angle) * right + std::sin(angle) * up;
        const float rebound = 45.0f + 28.0f * sample(impactRandom);
        const float scatter = 88.0f + 55.0f * sample(impactRandom);
        const Vector3 velocity = rebound * back + scatter * sideways + Vector3::UnitY * 22.0f;
        const float life = 0.23f + 0.13f * sample(impactRandom);
        const float length = 2.8f + 2.4f * sample(impactRandom);
        const float width = length * (0.35f + 0.22f * sample(impactRandom));
        const Color chipColor =
            Color::Lerp(Color(0.62f, 0.01f, 0.16f, 1), Color(0.98f, 0.045f, 0.36f, 1), sample(impactRandom));
        EmitParticle(particles, ImpactChip, position, velocity, life, Vector3(length, width, 1), chipColor);
    }
}

void CEmitter::EmitDefeat(ParticleManager& particles, const Collision::Sphere& sphere) {
    constexpr int ringSparkCount = 10;
    const float radius = std::clamp(sphere.radius, 8.0f, 35.0f);
    EmitParticle(particles, MuzzleFlash, sphere.center, Vector3::Zero, 0.16f,
                 Vector3(radius * 3.8f, radius * 1.4f, 1.0f), Color(0.66f, 1.0f, 0.94f, 1.0f));
    for (int i = 0; i < ringSparkCount; ++i) {
        const float angle = DirectX::XM_2PI * i / ringSparkCount;
        const Vector3 outward(std::cos(angle), 0.0f, std::sin(angle));
        const Vector3 origin = sphere.center + outward * (radius * 0.55f);
        const Vector3 velocity = outward * (95.0f + radius) + Vector3::UnitY * (35.0f + 35.0f * (i % 2));
        EmitParticle(particles, MuzzleSpark, origin, velocity, 0.33f, Vector3(radius * 0.9f),
                     Color(0.18f, 0.95f, 1.0f, 1.0f));
    }
}
