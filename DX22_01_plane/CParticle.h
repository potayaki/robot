/// @file CParticle.h
/// @brief 火花などの粒子の移動、寿命、共有描画資源。

#pragma once
/*
パーティクルのクラス
*/
#include "Object.h"

#include "PoolManager.h"

enum ParticleType {
    Test,
    Spark, // 火花
    MuzzleSpark, // 発射位置から飛び散る火花
    MuzzleFlash, // 発射位置に一瞬残る閃光
    MuzzlePlume, // 位置は銃口、速度は発射軸。Xが長さ、Yが最大幅。
    BulletRibbon, // 位置は実際の移動区間の中心、速度は軸。Xが全長、Yが幅。
    ImpactChip, // 速度方向を向く金属片。Xが長さ、Yが幅。
    ImpactRing, // 接点を囲むリング。XとYが消える直前の直径。
};

class CParticle : public Object {
private:
    /// 破片粒子に加わる重力加速度（ワールド単位/秒²）。
    float m_gravity = 72.0f;

    /// 破片粒子が回転する速さ（ラジアン/秒）。
    float m_spinSpeed = 6.0f;

    /// 基準の1フレームで火花のサイズに掛ける縮小率。
    float m_shrinkPerReferenceFrame = 0.95f;

    /// 旧フレーム単位の縮小率を秒単位へ換算する基準FPS。
    float m_referenceFramesPerSecond = 60.0f;

    /// 飛んでいく方向とスピード
    DirectX::SimpleMath::Vector3 m_velocity;
    /// 残りの寿命（秒）。
    float m_life = 1.0f;
    /// 発生時の寿命（秒）。発射エフェクトのフェード率に使う。
    float m_initialLife = 1.0f;
    /// パーティクルの種類
    ParticleType m_type = Test;

    /// パーティクルの色
    DirectX::SimpleMath::Color m_color;

    /// パーティクルがアクティブかどうか
    bool m_isActive = false;

public:
    CParticle();
    ~CParticle();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // 初速は距離/秒、寿命は秒で指定する。
    void SetVelocity(DirectX::SimpleMath::Vector3 vel) {
        m_velocity = vel;
    }

    void SetLife(float life) {
        m_life = life;
        m_initialLife = life;
    }

    void SetType(ParticleType type) {
        m_type = type;
    }

    void SetColor(DirectX::SimpleMath::Color color) {
        m_color = color;
    }

    bool GetActive() const {
        return m_isActive;
    }

    void SetActive(bool isActive) {
        m_isActive = isActive;
    }

    /// ステージ内の全粒子が共有する描画資源を初期化する。
    static void Preload();
    /// ステージの粒子プールを破棄した後に共有資源を解放する。
    static void UnloadResources();
};

using ParticleManager = PoolManager<CParticle, 5000>; // 最大5000個の粒子を再利用する。
