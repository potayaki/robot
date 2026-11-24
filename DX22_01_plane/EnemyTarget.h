/// @file EnemyTarget.h
/// @brief 敵の共通インターフェースと撃破時の経験値・ドロップ処理。

#pragma once

#include "Object.h"
#include "EnemyRarity.h"
#include "EnemyHitReaction.h"

// スライム・飛行ドローン・Boarなどを、攻撃やロックオンから同じ「敵」として扱うための共通型。
class EnemyTarget : public Object {
private:
    /// 通常種・亜種・希少種の区分。ボスは既定の通常種倍率で扱う。
    EnemyRarity m_rarity = EnemyRarity::Normal;
    /// この個体で最後に発生した命中の時刻と方向。
    EnemyHitReaction m_hitReaction;

protected:
    /// プレイ時間に合わせた、現在フレームの被弾姿勢。
    EnemyHitPose HitPose();
    // 全敵共通の撃破報酬。経験値とプレゼントを一度だけ付与する。
    void Defeat(int experience, int presentChance);
    // 攻撃元の整数ダメージを変更せず、現在つながっているGhostの倍率を適用する。
    int SupportedIncomingDamage(int damage) const;
    int SupportedOutgoingDamage(int damage) const;

public:
    /// 出現直後の個体へ種別を設定する。派生クラスは最大HPも初期化する。
    virtual void SetRarity(EnemyRarity rarity) {
        m_rarity = EnemyVariants::Normalize(rarity);
    }

    EnemyRarity GetRarity() const {
        return m_rarity;
    }

    virtual void OnHit(int& damage) = 0;
    /// 弾速や攻撃元からのベクトルで反応方向を決め、通常のダメージ処理を行う。
    void OnDirectionalHit(int& damage, const DirectX::SimpleMath::Vector3& incoming);
};
