/// @file CBullet.h
/// @brief 通常弾の発射、移動、誘導、移動区間での命中判定。

#pragma once
/*
Playerで使われる弾のクラス
*/
#include "TestModel.h"
#include "Object.h"
#include "PoolManager.h"
using namespace DirectX::SimpleMath;

class CBullet : public Object {
private:
    /// 発射してから弾を消すまでの時間（秒）。
    float m_lifetimeSeconds = 2.0f;

    /// 軌跡粒子を発生させる間隔（秒）。FPSによらず毎秒60個。
    float m_trailInterval = 1.0f / 60.0f;

    /// 軌跡の火花が消えるまでの時間（秒）。
    float m_trailLifetime = 1.0f / 3.0f;

    /// 軌跡の火花の大きさ。
    float m_trailScale = 2.5f;

    /// 弾モデルの幅・長さ・奥行きの倍率。
    Vector3 m_modelScale = {0.8f, 4.0f, 0.8f};

    /// 弾のモデル
    TestModel* m_model;
    /// 弾の移動方向と速さを含む速度ベクトル（ワールド単位/秒）。
    Vector3 m_velocity;
    /// 弾速（距離/秒）。旧25/フレームと同じ速さ。
    float m_speed = 1500.0f;
    /// 残りの寿命（秒）。
    float m_life = 0.0f;
    /// 軌跡の生成間隔も秒で管理する。
    float m_trailTimer = 0.0f;
    /// 弾のダメージ量
    int m_damage = 1;
    /// 発射時に設定し、飛翔中もPlayerの現在レベルへ揃える。0なら直進。
    int m_homingRank = 0;
    /// 弾の初期方向（例: 前方）
    Vector3 m_initialDirection{0.0f, 0.0f, 0.0f};

    /// 弾がアクティブかどうかのフラグ
    bool m_isActive = true;

    /// 当たり判定の半径
    float m_colRadius = 2.0f;

public:
    CBullet();
    ~CBullet();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override; //初期化処理
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override; //更新処理
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override; //描画処理
    /// 使用した資源を解放して終了する。
    void Uninit() override; //終了処理(モデルの破棄)

    bool GetActive() {
        return m_isActive;
    }

    void SetActive(bool active) {
        m_isActive = active;
    }

    void SetVelocity(Vector3 velocity) {
        m_velocity = velocity;
    }

    Vector3 GetVelocity() {
        return m_velocity;
    };

    // 最初に発射方向と初期位置をする関数
    void Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir, int homingRank = 0);

    //当たり判定用の球を返す関数
    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_colRadius};
    }

    //当たり判定用の球を返す関数
    void SetColRadius(float radius) {
        m_colRadius = radius;
    }

    // 発射時に現在の武器レベルの威力を引き継ぐ。
    void SetDamage(int value) {
        m_damage = value;
    }
};

using BulletManager = PoolManager<CBullet, 96>; // マルチショットと高速装填の最大強化にも対応。
