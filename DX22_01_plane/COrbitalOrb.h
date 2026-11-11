/// @file COrbitalOrb.h
/// @brief プレイヤーの周囲を回転して敵へダメージを与える守護オーブ。

#pragma once
#include "Object.h"
#include "TestCube.h"

class COrbitalOrb : public Object {
private:
    /// 敵との接触判定に使うオーブの半径。
    float m_collisionRadius = 8.0f;

    /// オーブの表示倍率。
    float m_modelScale = 0.5f;

    /// プレイヤー位置からオーブを浮かせる高さ。
    float m_heightOffset = 1.0f;

    /// 重なった敵へ再びダメージを与えるまでの間隔（秒）。
    float m_hitInterval = 0.5f;

    /// オーブの見た目
    TestCube* m_body;
    /// 中心となるプレイヤー
    Object* m_parentPlayer;
    /// 現在の角度（ラジアン）
    float m_angle;
    /// 回転半径
    float m_radius;
    /// 回転スピード
    float m_speed;

    /// 未初期化の値で攻撃が止まらないようにする。
    float m_cooldownTime = 0.0f;
    /// 攻撃が命中したときに与えるダメージ量。
    int m_damage = 1;

public:
    COrbitalOrb();
    ~COrbitalOrb();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // オーブとしての初期設定を行う関数
    void SetOrbitalParam(Object* player, float startAngle, float radius, float speed);

    void SetDamage(int damage) {
        m_damage = damage;
    }

    // 当たり判定
    Collision::Sphere GetCollisionSphere() override;
};
