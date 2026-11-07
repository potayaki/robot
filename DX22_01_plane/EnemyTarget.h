#pragma once

#include "Object.h"

// スライム・飛行ドローン・Boarなどを、攻撃やロックオンから同じ「敵」として扱うための共通型。
class EnemyTarget : public Object {
protected:
    // 全敵共通の撃破報酬。経験値とプレゼントを一度だけ付与する。
    void Defeat(int experience, int presentChance);
public:
    virtual void OnHit(int& damage) = 0;
};
