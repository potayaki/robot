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
    /// 出現時の種別を設定し、最大HPと接近速度を初期化する。自爆の予告時間は維持する。
    void SetRarity(EnemyRarity rarity) override;

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius};
    }

private:
    /// キノコの当たり判定半径。接地位置の計算にも使用する。
    float m_collisionRadius = 14.0f;

    /// 武器で撃破されたときに経験値オーブ10粒を落とす抽選の基礎重み。
    int m_orbDropWeight = 3;

    /// 武器で撃破されたときのプレゼント確率（%）。
    int m_presentChance = 2;

    /// このオブジェクトの見た目を描画するモデル。
    std::unique_ptr<MushroomModel> m_model;
    /// キノコの接近・起爆待ち・自爆の進行状態。
    MushroomFuse m_fuse;
    /// 通常種の最大HP。亜種・希少種の初期HPもこの値から計算する。
    static constexpr int BaseHp = 4;
    /// 現在の体力。出現時に種別の倍率を適用する。
    int m_hp = BaseHp;
    /// キノコの自爆が届く半径（ワールド単位）。
    static constexpr float BlastRadius = 65.0f;
    /// キノコの自爆がプレイヤーへ与えるダメージ量。
    static constexpr int BlastDamage = 8;
};
