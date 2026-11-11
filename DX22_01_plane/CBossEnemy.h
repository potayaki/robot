/// @file CBossEnemy.h
/// @brief 大型スライムボスの追跡、予備動作、突進、被弾。

#pragma once
#include "EnemyTarget.h"
#include "TestModel.h"
#include <memory>

// EnemyTargetを継承し、通常弾・ミサイル・全自動武器の既存判定を利用する。
class CBossEnemy : public EnemyTarget {
public:
    enum class Phase {
        Approach,
        Windup,
        Charge,
        Recover
    };

private:
    /// ボスの判定半径。モデルの地面合わせにも使用する。
    float m_collisionRadius = 35.0f;

    /// 通常追跡の移動速度。
    float m_approachSpeed = 22.0f;

    /// 通常追跡でプレイヤーとの間に残す距離。
    float m_stopDistance = 37.0f;

    /// 突進中の移動速度。
    float m_chargeSpeed = 150.0f;

    /// 追跡フェーズの長さ（秒）。
    float m_approachSeconds = 2.0f;

    /// 突進前の予備動作の長さ（秒）。
    float m_windupSeconds = 1.0f;

    /// 突進フェーズの長さ（秒）。
    float m_chargeSeconds = 0.85f;

    /// 突進後の硬直の長さ（秒）。
    float m_recoverySeconds = 1.0f;

    /// 接触ダメージの最短間隔（秒）。
    float m_contactInterval = 0.8f;

    /// 突進で命中したときのダメージ量。
    int m_chargeDamage = 15;

    /// 通常の接触時のダメージ量。
    int m_contactDamage = 8;

    /// ボス撃破時の経験値。
    int m_experience = 15;

    /// ボス撃破時のプレゼント抽選確率（パーセント）。
    int m_presentChance = 100;

    /// キャラクターや弾の本体を描画するモデル。
    std::unique_ptr<TestModel> m_body;
    /// 現在の体力。0以下になると死亡処理を行う。
    int m_hp = 250;
    /// 最大HP。HPバーの割合の計算にも使用する。
    int m_maxHp = 250;
    /// 現在の攻撃フェーズ。
    Phase m_phase = Phase::Approach;
    /// 現在の攻撃フェーズが終了するまでの残り秒数。
    float m_phaseRemaining = 2.0f;
    /// 接触ダメージを再び与えられるまでの残り秒数。
    float m_contactCooldown = 0.0f;
    /// 予備動作の終了時に固定した突進方向。
    DirectX::SimpleMath::Vector3 m_chargeDirection{0, 0, 1};
    void UpdateModel();

public:
    ~CBossEnemy() override {
        Uninit();
    }

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
    void OnHit(int& damage) override;
    void Configure(int hp, const DirectX::SimpleMath::Vector3& position);

    int GetHp() const {
        return (std::max)(0, m_hp);
    }

    int GetMaxHp() const {
        return m_maxHp;
    }

    Phase GetPhase() const {
        return m_phase;
    }

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius};
    }
};
