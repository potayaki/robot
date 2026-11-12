/// @file EnemyTarget.h
/// @brief 敵の共通インターフェースと撃破時の経験値・ドロップ処理。

#pragma once

#include "Object.h"

// スライム・飛行ドローン・Boarなどを、攻撃やロックオンから同じ「敵」として扱うための共通型。
class EnemyTarget : public Object {
protected:
    // 全敵共通の撃破報酬。経験値とプレゼントを一度だけ付与する。
    void Defeat(int experience, int presentChance);
    // 攻撃元の整数ダメージを変更せず、現在つながっているGhostの倍率を適用する。
    int SupportedIncomingDamage(int damage) const;
    int SupportedOutgoingDamage(int damage) const;

public:
    virtual void OnHit(int& damage) = 0;
};
