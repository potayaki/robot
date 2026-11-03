#pragma once
#include "EnemyTarget.h"
#include "TestModel.h"
#include <memory>

// EnemyTargetを継承し、通常弾・ミサイル・全自動武器の既存判定を利用する。
class CBossEnemy : public EnemyTarget
{
public:
    enum class Phase { Approach, Windup, Charge, Recover };

private:
    std::unique_ptr<TestModel> m_body;
    int m_hp = 250;
    int m_maxHp = 250;
    Phase m_phase = Phase::Approach;
    float m_phaseRemaining = 2.0f;
    float m_contactCooldown = 0.0f;
    DirectX::SimpleMath::Vector3 m_chargeDirection{0, 0, 1};
    void UpdateModel();

public:
    ~CBossEnemy() override { Uninit(); }
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;
    void OnHit(int& damage) override;
    void Configure(int hp, const DirectX::SimpleMath::Vector3& position);

    int GetHp() const { return (std::max)(0, m_hp); }
    int GetMaxHp() const { return m_maxHp; }
    Phase GetPhase() const { return m_phase; }
    Collision::Sphere GetCollisionSphere() override { return {m_Position, 35.0f}; }
};
