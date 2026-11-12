/// @file CGhostEnemy.h
/// @brief 仲間への支援、距離維持、孤立時の霊弾を行う敵。
#pragma once
#include "EnemyTarget.h"
#include "GhostModel.h"
#include "GhostSupportRules.h"

class CGhostEnemy : public EnemyTarget {
public:
    void Init() override;
    void Update() override;
    void Draw(Camera* camera) override;
    void Uninit() override;
    void OnHit(int& damage) override;

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, CollisionRadius};
    }

    void DrawEffects(Camera* camera);
    std::vector<EnemyTarget*> GetSupportTargets() const;
    static GhostSupport::Settings& SupportSettings();
#ifdef _DEBUG
    static void DrawDebugUI();
#endif

private:
    struct SpiritShot {
        /// 霊弾の現在位置（ワールド座標）。
        DirectX::SimpleMath::Vector3 position;
        /// 発射時に確定する速度（ワールド単位/秒）。追尾しない。
        DirectX::SimpleMath::Vector3 velocity;
        /// 弾の残り寿命（秒）。
        float remaining = 4.0f;
    };

    /// Ghostの判定半径。見た目の高さは32。
    static constexpr float CollisionRadius = 16.0f;
    /// 霊弾の判定半径（ワールド単位）。
    static constexpr float ShotRadius = 6.0f;
    /// 他のGhost・スポナーと共有する描画資源。
    std::shared_ptr<GhostModel> m_model;
    /// 生存中の霊弾。Ghost撃破時に消す。
    std::vector<SpiritShot> m_shots;
    /// 現在の体力。
    int m_hp = 6;
    /// プレイヤーから離れ始める水平距離（ワールド単位）。
    float m_nearDistance = 180.0f;
    /// この水平距離を超えたら接近する（ワールド単位）。
    float m_farDistance = 260.0f;
    /// 水平移動速度（ワールド単位/秒）。
    float m_moveSpeed = 65.0f;
    /// 地面からGhost中心までの高さ（ワールド単位）。
    float m_hoverHeight = 36.0f;
    /// 浮遊動作の経過秒数。
    float m_elapsed = 0;
    /// 次の霊弾までの残り秒数。支援中はリセットする。
    float m_shotCooldown = 1.5f;
    /// 霊弾の発射間隔（秒）。
    float m_shotInterval = 2.0f;
    /// 霊弾の速度（ワールド単位/秒）。
    float m_shotSpeed = 120.0f;
    /// 霊弾を発射できる距離（ワールド単位）。
    float m_shotRange = 500.0f;
    /// 霊弾1発の基本ダメージ。
    int m_shotDamage = 4;
};
