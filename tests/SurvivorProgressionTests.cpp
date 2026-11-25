/// @file SurvivorProgressionTests.cpp
/// @brief 動作と境界条件を検証する SurvivorProgressionTests の自動テスト。

#include "../DX22_01_plane/SurvivorProgression.h"
#include <cassert>
#include "../DX22_01_plane/BulletHoming.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <set>

namespace {
    void CheckOrbMagnetRewards() {
        SurvivorProgression growth(124);
        const auto id = UpgradeId::OrbMagnet;
        const int maxRank = GetUpgradeDefinition(id).maxRank;
        assert(maxRank == 5);
        // 実際のレベルアップ候補から最大まで選ぶ。経験値倍率を変える学習回路は選ばない。
        while (growth.Rank(id) < maxRank) {
            growth.AddExperience(growth.RequiredExperience() / UpgradeBalance::ExperienceUnit);
            const auto& choices = growth.Choices();
            auto choice = std::find(choices.begin(), choices.end(), id);
            if (choice == choices.end()) {
                choice = std::find_if(choices.begin(), choices.end(), [](UpgradeId candidate) {
                    return candidate != UpgradeId::Experience && candidate != UpgradeId::Heal;
                });
            }
            UpgradeId granted;
            assert(choice != choices.end() && growth.Select(choice - choices.begin(), granted));
            if (granted == id) {
                const int rank = growth.Rank(id);
                const auto radius = static_cast<int>(UpgradeBalance::OrbMagnetRadius(rank));
                assert(GetUpgradeEffectText(id, rank).find(std::to_string(radius)) != std::string::npos);
            }
        }
        growth.AddExperience(1);
        assert(growth.Experience() == UpgradeBalance::ExperienceUnit);
    }

    void CheckEmergencyShield() {
        const auto id = UpgradeId::EmergencyShield;
        const int maxRank = GetUpgradeDefinition(id).maxRank;
        assert(maxRank == 3);
        for (int rank = 0; rank <= maxRank; ++rank) {
            const float seconds = UpgradeBalance::DamageInvincibilitySeconds(rank);
            assert(std::abs(seconds - (1.0f / 3.0f + 0.15f * rank)) < 0.0001f);
            char displaySeconds[16];
            std::snprintf(displaySeconds, sizeof(displaySeconds), "%.2f", seconds);
            assert(GetUpgradeEffectText(id, rank).find(displaySeconds) != std::string::npos);
        }
    }
} // namespace

int main() {
    CheckOrbMagnetRewards();
    CheckEmergencyShield();
    // 全レベルで攻撃力・射程が上昇し、攻撃間隔が短縮される。表示文も単体追従仕様。
    auto previousTurret = GetTurretStats(1);
    for (int rank = 1; rank <= 5; ++rank) {
        const auto stats = GetTurretStats(rank);
        assert(stats.damage == rank);
        assert(stats.attackInterval > 0.2f);
        if (rank > 1) {
            assert(stats.damage > previousTurret.damage && stats.range > previousTurret.range);
            assert(stats.attackInterval < previousTurret.attackInterval);
        }
        const auto description = GetUpgradeEffectText(UpgradeId::Turret, rank);
        assert(description.find(reinterpret_cast<const char*>(u8"相棒1体")) != std::string::npos);
        assert(description.find(reinterpret_cast<const char*>(u8"設置間隔")) == std::string::npos);
        previousTurret = stats;
    }
#ifdef _DEBUG
    // デバッグ取得は任意のスキルを選べるが、通常の上限と選択待ち回数を守る。
    SurvivorProgression debugGrowth(19);
    assert(!debugGrowth.DebugGrantUpgrade(UpgradeId::Count));
    assert(!debugGrowth.DebugGrantUpgrade(UpgradeId::Heal));
    assert(!debugGrowth.DebugGrantUpgrade(static_cast<UpgradeId>(-1)));
    debugGrowth.AddExperience(UpgradeBalance::InitialRequiredExperience + 1);
    const int experienceBefore = debugGrowth.Experience();
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
        const auto id = static_cast<UpgradeId>(i);
        const int maxRank = GetUpgradeDefinition(id).maxRank;
        for (int rank = 1; rank <= maxRank; ++rank) {
            assert(debugGrowth.DebugGrantUpgrade(id));
            assert(debugGrowth.Rank(id) == rank);
        }
        assert(!debugGrowth.DebugGrantUpgrade(id));
        assert(debugGrowth.Rank(id) == maxRank);
        assert(debugGrowth.Level() == 2 && debugGrowth.PendingChoices() == 1);
        assert(debugGrowth.Experience() == experienceBefore);
        for (const auto choice : debugGrowth.Choices()) {
            assert(choice == UpgradeId::Heal ||
                   debugGrowth.Rank(choice) < GetUpgradeDefinition(choice).maxRank);
        }
    }
    assert(debugGrowth.Choices().size() == 1 && debugGrowth.Choices()[0] == UpgradeId::Heal);

    // 下げる操作も通常の下限を守り、報酬・経験値を巻き戻さない。
    const auto maxedChoices = debugGrowth.Choices();
    assert(!debugGrowth.DebugRemoveUpgrade(UpgradeId::Count));
    assert(!debugGrowth.DebugRemoveUpgrade(UpgradeId::Heal));
    assert(!debugGrowth.DebugRemoveUpgrade(static_cast<UpgradeId>(-1)));
    assert(debugGrowth.Choices() == maxedChoices);
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
        const auto id = static_cast<UpgradeId>(i);
        const int maxRank = GetUpgradeDefinition(id).maxRank;
        for (int rank = maxRank - 1; rank >= 0; --rank) {
            assert(debugGrowth.DebugRemoveUpgrade(id));
            assert(debugGrowth.Rank(id) == rank);
            assert(debugGrowth.Level() == 2 && debugGrowth.PendingChoices() == 1);
            assert(debugGrowth.Experience() == experienceBefore);
            // 他は最大なので、戻したスキルだけが再び報酬候補になる。
            assert(debugGrowth.Choices().size() == 1 && debugGrowth.Choices()[0] == id);
        }
        const auto zeroChoices = debugGrowth.Choices();
        assert(!debugGrowth.DebugRemoveUpgrade(id) && debugGrowth.Rank(id) == 0);
        assert(debugGrowth.Choices() == zeroChoices);
        for (int rank = 1; rank <= maxRank; ++rank) {
            assert(debugGrowth.DebugGrantUpgrade(id));
            assert(debugGrowth.Rank(id) == rank);
        }
        assert(debugGrowth.Choices().size() == 1 && debugGrowth.Choices()[0] == UpgradeId::Heal);
    }
    SurvivorProgression noPending(20);
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
        const auto id = static_cast<UpgradeId>(i);
        assert(!noPending.DebugRemoveUpgrade(id));
        assert(noPending.DebugGrantUpgrade(id));
        assert(noPending.DebugRemoveUpgrade(id));
        assert(noPending.Rank(id) == 0);
        assert(noPending.Level() == 1 && noPending.Experience() == 0);
        assert(noPending.PendingChoices() == 0 && noPending.Choices().empty());
    }
#endif
    SurvivorProgression growth(7);
    UpgradeId reward = UpgradeId::Heal;
    assert(!growth.Select(0, reward));
    growth.AddExperience(-5);
    // 敵1体から36 EXPを回収したときは、戦闘を続けたままゲージが36/40まで進む。
    growth.AddExperience(36);
    assert(growth.Level() == 1 && !growth.IsChoosing() && growth.Experience() == 3600);
    assert(growth.RequiredExperience() == 4000);
    growth.AddExperience(3);
    assert(growth.Level() == 1 && growth.Experience() == 3900);
    growth.AddExperience(1);
    assert(growth.Level() == 2 && growth.Experience() == 0 && growth.PendingChoices() == 1);
    constexpr int secondThreshold =
        UpgradeBalance::InitialRequiredExperience + UpgradeBalance::RequiredExperiencePerLevel;
    assert(growth.RequiredExperience() == secondThreshold * UpgradeBalance::ExperienceUnit);
    assert(growth.Choices().size() == 3);
    assert(std::set<UpgradeId>(growth.Choices().begin(), growth.Choices().end()).size() == 3);
    const auto offered = growth.Choices();
    growth.AddExperience(1);
    assert(growth.Choices() == offered);
    assert(!growth.Select(3, reward));
    assert(growth.Select(0, reward) && growth.Rank(reward) == 1 && !growth.IsChoosing());

    // Multiple level-ups must retain overflow XP and require one selection per level.
    SurvivorProgression burst(42);
    constexpr int thirdThreshold = secondThreshold + UpgradeBalance::RequiredExperiencePerLevel;
    burst.AddExperience(UpgradeBalance::InitialRequiredExperience + secondThreshold + thirdThreshold + 5);
    assert(burst.Level() == 4 && burst.Experience() == 500 && burst.PendingChoices() == 3);
    assert(burst.RequiredExperience() ==
           (thirdThreshold + UpgradeBalance::RequiredExperiencePerLevel) * UpgradeBalance::ExperienceUnit);
    for (int pending = 3; pending > 0; --pending) {
        assert(burst.Select(0, reward));
        assert(burst.PendingChoices() == pending - 1);
    }
    assert(burst.Choices().empty());

    // Presents never consume pending level rewards; exhausted upgrades are excluded.
    SurvivorProgression chest(17);
    chest.AddExperience(UpgradeBalance::InitialRequiredExperience);
    for (int i = 0; i < 200; ++i) {
        const auto item = chest.GrantPresent();
        assert(chest.PendingChoices() == 1 && chest.Level() == 2);
        if (item != UpgradeId::Heal) {
            assert(chest.Rank(item) <= GetUpgradeDefinition(item).maxRank);
        }
        for (auto choice : chest.Choices()) {
            assert(choice == UpgradeId::Heal || chest.Rank(choice) < GetUpgradeDefinition(choice).maxRank);
        }
    }
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
        auto id = static_cast<UpgradeId>(i);
        assert(GetUpgradeDefinition(id).id == id);
        assert(chest.Rank(id) == GetUpgradeDefinition(id).maxRank);
        // 全スキルの具体的な効果が、最初の取得時・最大強化時とも表示できる。
        assert(!GetUpgradeEffectText(id, 1).empty());
        assert(!GetUpgradeEffectText(id, GetUpgradeDefinition(id).maxRank).empty());
    }
    assert(GetUpgradeDefinition(UpgradeId::BulletHoming).maxRank == 5);
    assert(chest.Rank(UpgradeId::BulletHoming) == 5); // プレゼントの共通抽選にも含まれる。
    assert(GetUpgradeEffectText(UpgradeId::BulletHoming, 1)
               .find(std::to_string(static_cast<int>(BulletHoming::DegreesPerSecond(1)))) !=
           std::string::npos);
    assert(GetUpgradeEffectText(UpgradeId::BulletHoming, 5)
               .find(std::to_string(static_cast<int>(BulletHoming::DegreesPerSecond(5)))) !=
           std::string::npos);
    assert(GetUpgradeEffectText(UpgradeId::RapidFire, 1).find("0.085") != std::string::npos);
    assert(chest.Choices().size() == 1 && chest.Choices()[0] == UpgradeId::Heal);
    assert(chest.Select(0, reward) && reward == UpgradeId::Heal);

    // Fractional XP bonuses must accumulate even when each kill awards only one XP.
    SurvivorProgression bonus(91);
    while (bonus.Rank(UpgradeId::Experience) == 0) {
        bonus.GrantPresent();
    }
    bonus.AddExperience(1);
    assert(bonus.Experience() == 120);

    // A new run has no upgrades or pending rewards, even after a maxed-out previous run.
    SurvivorProgression fresh(1);
    assert(fresh.Level() == 1 && fresh.Experience() == 0 && !fresh.IsChoosing());
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
        assert(fresh.Rank(static_cast<UpgradeId>(i)) == 0);
    }
    std::cout << "Survivor progression tests passed.\n";
}
