/// @brief 各エリアのボス撃破、3つの門の開門演出、城内の最終戦の進行。
#pragma once
#include "BossCatalog.h"
#include "ForestLayout.h"
#include <algorithm>
#include <array>

class ForestGateProgress {
    /// 各門に対応するエリアのボスを順に倒したか。
    std::array<bool, ForestLayout::GateCount> m_unlocked{};
    /// 各門の開門演出の進行率（0～1）。
    std::array<float, ForestLayout::GateCount> m_openAmount{};
    /// 城内でマッシュルームキングを倒し、最後のゴーストロードへ進んだか。
    bool m_keepMushroomDefeated = false;
    /// 城内の最後のボスを倒してステージを完了したか。
    bool m_complete = false;

public:
    static constexpr float OpeningSeconds = 2.4f;

    void OnBossDefeated(BossKind kind, ForestLayout::Area area = ForestLayout::Area::Entrance) {
        const size_t index = static_cast<size_t>(area);
        if (index > static_cast<size_t>(OpenGateCount()) || IsAreaCleared(area) || kind != NextBoss(area)) {
            return;
        }
        if (area == ForestLayout::Area::Keep) {
            if (kind == BossKind::Mushroom) {
                m_keepMushroomDefeated = true;
            } else {
                m_complete = true;
            }
        } else {
            m_unlocked[index] = true;
        }
    }

    void Update(float seconds) {
        for (size_t gate = 0; gate < m_openAmount.size(); ++gate) {
            if (m_unlocked[gate]) {
                m_openAmount[gate] =
                    std::clamp(m_openAmount[gate] + (std::max)(0.0f, seconds) / OpeningSeconds, 0.0f, 1.0f);
            }
        }
    }

    bool IsUnlocked(size_t index = 0) const {
        return index < m_unlocked.size() && m_unlocked[index];
    }

    bool IsOpen(size_t index = 0) const {
        return index < m_openAmount.size() && m_openAmount[index] >= 1.0f;
    }

    float OpenAmount(size_t index = 0) const {
        return index < m_openAmount.size() ? m_openAmount[index] : 0.0f;
    }

    int OpenGateCount() const {
        int count = 0;
        while (count < ForestLayout::GateCount && IsOpen(count)) {
            ++count;
        }
        return count;
    }

    bool IsAreaCleared(ForestLayout::Area area) const {
        return area == ForestLayout::Area::Keep ? m_complete : IsUnlocked(static_cast<size_t>(area));
    }

    BossKind NextBoss(ForestLayout::Area area) const {
        constexpr std::array bosses{BossKind::Slime, BossKind::Fly, BossKind::Boar, BossKind::Mushroom};
        if (area == ForestLayout::Area::Keep && m_keepMushroomDefeated) {
            return BossKind::Ghost;
        }
        return bosses[static_cast<size_t>(area)];
    }

    bool IsComplete() const {
        return m_complete;
    }
};
