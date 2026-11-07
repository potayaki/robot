#pragma once

#include "EnemyTarget.h"
#include "BoarCharge.h"
#include "BoarModel.h"

// 地上を追いかけ、赤い予測線で方向を知らせてから一直線に突進する敵。
class CBoarEnemy : public EnemyTarget
{
public:
    static constexpr float DefaultChargeStartRange = 170.0f;

    // Playerとの水平距離がこの値以下になると予備動作を開始する。
    // 調整したい「突進を始める範囲」はこのメンバ変数。
    float m_chargeStartRange = DefaultChargeStartRange;

    void Init() override;
    void Update() override;
    void Draw(Camera* camera) override;
    void DrawTelegraph(Camera* camera);
    void Uninit() override;
    void OnHit(int& damage) override;
    Collision::Sphere GetCollisionSphere() override { return {m_Position, m_collisionRadius}; }
#ifdef _DEBUG
    static void DrawDebugUI();
#endif

private:
    std::unique_ptr<BoarModel> m_model;
    BoarCharge m_charge;
    int m_hp = 8;
    int m_chargeDamage = 6;
    int m_experience = 4;
    int m_presentChance = 12;
    float m_collisionRadius = 14.0f;
    float m_modelScale = 8.0f; // 元の高さ約3.5を、ゲーム内の高さ約28へ合わせる。
    float m_groundY = -5.0f;
};
