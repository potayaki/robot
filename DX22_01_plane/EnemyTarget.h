#pragma once

#include "Object.h"

// スライムと飛行ドローンを、攻撃やロックオンから同じ「敵」として扱うための共通型。
class EnemyTarget : public Object {
public:
    virtual void OnHit(int& damage) = 0;
};
