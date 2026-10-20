#pragma once
#include <array>
#include <random>
#include <vector>
#include <string>

// 描画やDirectXから独立した成長ルール。抽選・上限・複数レベル分の選択をここで管理する。
enum class UpgradeId {
    BulletPower, RapidFire, MultiShot, MissilePower, MissileVolley, MissileReload,
    Orbit, Turret, Shockwave, Lightning, MoveSpeed, Vitality, Armor, Regeneration,
    Experience, BulletHoming, Count, Heal
};

struct UpgradeDefinition {
    UpgradeId id;
    const char8_t* name;
    const char8_t* description;
    int maxRank;
};

const UpgradeDefinition& GetUpgradeDefinition(UpgradeId id);
// 所持一覧と報酬候補の両方に、指定レベルの具体的な性能を表示する。
std::string GetUpgradeEffectText(UpgradeId id, int rank);

class SurvivorProgression {
    std::array<int, static_cast<size_t>(UpgradeId::Count)> m_ranks{};
    std::mt19937 m_random;
    int m_level = 1;
    int m_experience = 0; // 100で経験値1。経験値ボーナスの小数部分も失わない。
    int m_pendingChoices = 0;
    std::vector<UpgradeId> m_choices;
    std::vector<UpgradeId> EligibleUpgrades() const;
    void RefreshChoices();
    void Grant(UpgradeId id);
public:
    explicit SurvivorProgression(unsigned seed = std::random_device{}()) : m_random(seed) {}
    int Rank(UpgradeId id) const;
    int Level() const { return m_level; }
    int Experience() const { return m_experience; }
    int RequiredExperience() const { return (5 + (m_level - 1) * 3) * 100; }
    int PendingChoices() const { return m_pendingChoices; }
    bool IsChoosing() const { return m_pendingChoices > 0; }
    const std::vector<UpgradeId>& Choices() const { return m_choices; }
    void AddExperience(int amount);
    bool Select(size_t index, UpgradeId& granted);
    UpgradeId GrantPresent();
};
