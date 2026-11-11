/// @file CMushroomEnemy.h
/// @brief 接近後に予告点滅して自爆するキノコの敵。

#pragma once
#include "EnemyTarget.h"
#include "MushroomFuse.h"
#include "MushroomModel.h"

class CMushroomEnemy : public EnemyTarget {
public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera) override;

    /// 使用した資源を解放して終了する。
    void Uninit() override {
        m_model.reset();
    }

    void OnHit(int& damage) override;

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius};
    }

private:
    /// キノコの当たり判定半径。接地位置の計算にも使用する。
    float m_collisionRadius = 14.0f;

    /// 武器で撃破されたときに渡す経験値。
    int m_experience = 3;

    /// 武器で撃破されたときのプレゼント確率（%）。
    int m_presentChance = 10;

    /// このオブジェクトの見た目を描画するモデル。
    std::unique_ptr<MushroomModel> m_model;
    /// キノコの接近・起爆待ち・自爆の進行状態。
    MushroomFuse m_fuse;
    /// 現在の体力。0以下になると死亡処理を行う。
    int m_hp = 4;
    /// キノコの自爆が届く半径（ワールド単位）。
    static constexpr float BlastRadius = 65.0f;
    /// キノコの自爆がプレイヤーへ与えるダメージ量。
    static constexpr int BlastDamage = 8;
};
