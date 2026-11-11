/// @file CBoarEnemy.h
/// @brief 突進型の敵Boarと、その死亡演出。

#pragma once

#include "EnemyTarget.h"
#include "BoarCharge.h"
#include "BoarModel.h"

// 地上を追いかけ、赤い予測線で方向を知らせてから一直線に突進する敵。
class CBoarEnemy : public EnemyTarget {
public:
    /// Boarが予備動作を始めるプレイヤーとの標準距離。
    static constexpr float DefaultChargeStartRange = 170.0f;

    // Playerとの水平距離がこの値以下になると予備動作を開始する。
    // 調整したい「突進を始める範囲」はこのメンバ変数。
    /// この距離以内へ近づくと、突進の予備動作を始める。
    float m_chargeStartRange = DefaultChargeStartRange;

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera) override;
    void DrawTelegraph(Camera* camera);
    /// 使用した資源を解放して終了する。
    void Uninit() override;
    void OnHit(int& damage) override;

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius};
    }
#ifdef _DEBUG
    static void DrawDebugUI();
#endif

private:
    /// このオブジェクトの見た目を描画するモデル。
    std::unique_ptr<BoarModel> m_model;
    /// Boarの接近・予備動作・突進・硬直を管理する状態。
    BoarCharge m_charge;
    /// 現在の体力。0以下になると死亡処理を行う。
    int m_hp = 8;
    /// 突進がプレイヤーへ命中したときに与えるダメージ量。
    int m_chargeDamage = 6;
    /// 撃破時にプレイヤーへ渡す経験値。
    int m_experience = 4;
    /// 撃破時にプレゼントを落とす確率（パーセント）。
    int m_presentChance = 12;
    /// 当たり判定の球の半径（ワールド単位）。
    float m_collisionRadius = 14.0f;
    /// 元の高さ約3.5を、ゲーム内の高さ約28へ合わせる。
    float m_modelScale = 8.0f;
    /// 足元や破片の着地に使用する地面の高さ。
    float m_groundY = -5.0f;
};
