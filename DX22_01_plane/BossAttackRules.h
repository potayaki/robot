/// @brief ボス攻撃の秒単位の進行。予告と攻撃の境界で更新を区切る。
#pragma once
#include "BossCatalog.h"
#include <algorithm>

enum class BossAttackPhase {
    Approach,
    Windup,
    Charge,
    Recover
};

class BossAttackCycle {
public:
    void Reset(BossKind kind) {
        m_kind = kind;
        m_phase = BossAttackPhase::Approach;
        m_chargeCount = 0;
        m_remaining = Duration();
    }

    BossAttackPhase Phase() const {
        return m_phase;
    }

    float Remaining() const {
        return m_remaining;
    }

    float Progress() const {
        return 1.0f - m_remaining / Duration();
    }

    float Duration() const {
        // 追跡、予告、攻撃、硬直の秒数。Boarは予告付きの突進を2回行う。
        constexpr float seconds[BossTypeCount][4] = {{2.0f, 1.0f, 0.85f, 1.0f},
                                                     {2.2f, 1.1f, 0.9f, 1.5f},
                                                     {1.6f, 0.9f, 1.1f, 1.8f},
                                                     {2.0f, 1.5f, 2.6f, 1.5f},
                                                     {2.0f, 1.2f, 0.8f, 1.4f}};
        return seconds[static_cast<int>(m_kind)][static_cast<int>(m_phase)];
    }

    /// 現フェーズで消費した秒数。呼び出し元がこの時間だけ移動・命中を処理する。
    float Consume(float seconds) {
        const float step = std::clamp(seconds, 0.0f, m_remaining);
        m_remaining = (std::max)(0.0f, m_remaining - step);
        return step;
    }

    /// 残り時間が0になった時だけ次の動作へ移る。
    bool Advance() {
        if (m_remaining > 0.000001f) {
            return false;
        }
        switch (m_phase) {
            case BossAttackPhase::Approach:
                m_phase = BossAttackPhase::Windup;
                m_chargeCount = 0;
                break;
            case BossAttackPhase::Windup:
                m_phase = BossAttackPhase::Charge;
                ++m_chargeCount;
                break;
            case BossAttackPhase::Charge:
                m_phase = m_kind == BossKind::Boar && m_chargeCount < 2 ? BossAttackPhase::Windup
                                                                        : BossAttackPhase::Recover;
                break;
            case BossAttackPhase::Recover:
                m_phase = BossAttackPhase::Approach;
                break;
        }
        m_remaining = Duration();
        return true;
    }

private:
    /// 攻撃パターンを選ぶ種族。
    BossKind m_kind = BossKind::Slime;
    /// 現在の攻撃動作。
    BossAttackPhase m_phase = BossAttackPhase::Approach;
    /// 現在の動作の残り秒数。
    float m_remaining = 2.0f;
    /// 今回の連続攻撃で開始した突進の回数。
    int m_chargeCount = 0;
};
