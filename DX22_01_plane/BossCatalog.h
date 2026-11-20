/// @file BossCatalog.h
/// @brief ボスの種類、出現時の体格と、このプレイの撃破記録。
#pragma once
#include <array>
#include <bitset>

enum class BossKind {
    Slime,
    Fly,
    Boar,
    Mushroom,
    Ghost,
    Count
};
inline constexpr int BossTypeCount = static_cast<int>(BossKind::Count);

inline constexpr const char8_t* BossName(BossKind kind) {
    constexpr std::array names{u8"スライムキング", u8"フライクイーン", u8"イノシシキング",
                               u8"マッシュルームキング", u8"ゴーストロード"};
    return names[static_cast<size_t>(kind)];
}

inline constexpr float BossRadius(BossKind kind) {
    constexpr std::array radii{35.0f, 32.0f, 40.0f, 36.0f, 30.0f};
    return radii[static_cast<size_t>(kind)];
}

inline constexpr float BossHeight(BossKind kind) {
    constexpr std::array heights{35.0f, 95.0f, 40.0f, 36.0f, 55.0f};
    return heights[static_cast<size_t>(kind)];
}

class BossRoster {
    /// このプレイで撃破した種類。同じ種類を倒しても重複加算しない。
    std::bitset<BossTypeCount> m_defeated;
    /// 次の戦闘で出現する種類。撃破後に次種へ進み、全種の後は循環する。
    BossKind m_next = BossKind::Slime;

public:
    BossKind Next() const {
        return m_next;
    }

    int DefeatedCount() const {
        return static_cast<int>(m_defeated.count());
    }

    void SelectNext(BossKind kind) {
        m_next = kind;
    }

    void RecordDefeat(BossKind kind) {
        m_defeated.set(static_cast<size_t>(kind));
        m_next = static_cast<BossKind>((static_cast<int>(kind) + 1) % BossTypeCount);
    }
};
