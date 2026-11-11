/// @file CEnemy.h
/// @brief 通常・高速・大型スライムの移動、接触攻撃、撃破報酬。

#pragma once
/*
キャラクターの敵のクラス
*/
//#include "TestCube.h"
#include "TestModel.h"
#include "EnemyTarget.h"
// 同じモデルを使っても、体格・移動・攻撃性能で異なる役割を持たせる。
enum class SlimeVariant {
    Normal,
    Runner,
    Brute
};

class CEnemy : public EnemyTarget {
private:
    /// 通常サイズのスライムモデルに掛ける倍率。
    float m_modelScale = 0.2f;

    /// 素材の向きを合わせるX軸回転（ラジアン）。
    float m_modelPitch = -30.0f;

    /// 通常サイズの判定半径。体格倍率を掛けて使用する。
    float m_collisionRadius = 10.0f;

    /// 高速型のダッシュを繰り返す周期（秒）。
    float m_dashPeriod = 2.5f;

    /// 高速型が周期の先頭で加速する時間（秒）。
    float m_dashDuration = 25.0f / 60.0f;

    /// 高速型のダッシュ中に掛ける移動速度の倍率。
    float m_dashMultiplier = 1.7f;

    /// 接触ダメージを与える間隔（秒）。
    float m_contactInterval = 1.0f;

    //敵の体
    /// キャラクターや弾の本体を描画するモデル。
    TestModel* m_body;

    /// 現在の体力。0以下になると撃破報酬を渡す。
    int m_hp = 3;

    /// 移動速度（距離/秒）。プレイヤーは60。
    float m_speed = 30.0f;

    /// 地面に接しているか。空中ではfalseにする。
    bool m_isGrounded = false;

    /// プレゼントのドロップ率
    int m_dropPercent = 1;

    /// プレイヤーに与える接触攻撃のダメージ量。
    int m_damage = 2;

    /// 攻撃のクールダウンタイマー
    float m_attackTimer = 0.0f;
    /// 通常型・高速型・大型のスライムの種類。
    SlimeVariant m_variant = SlimeVariant::Normal;
    /// 標準モデルに対する体格の倍率。見た目と判定半径に使用する。
    float m_size = 1.0f;
    /// 撃破時にプレイヤーへ渡す経験値。
    int m_experience = 1;
    /// ダッシュ周期内の経過秒数。
    float m_moveTime = 0.0f;

public:
    CEnemy();
    ~CEnemy();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    void Onland();
    void OnHit(int& damage) override; //弾に当たったときの処理
    void SetVariant(SlimeVariant variant);

    void SetPosition(float x, float y, float z) {
        DirectX::SimpleMath::Vector3 p = {x, y, z};
        m_position = p;
    }

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius * m_size}; // 大型・小型の見た目に判定も合わせる。
    }
};
