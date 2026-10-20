#include "../DX22_01_plane/SurvivorProgression.h"
#include <cassert>
#include <iostream>
#include <set>

int main() {
    SurvivorProgression growth(7);
    UpgradeId reward = UpgradeId::Heal;
    assert(!growth.Select(0, reward));
    growth.AddExperience(-5);
    growth.AddExperience(4);
    assert(growth.Level() == 1 && growth.Experience() == 400);
    growth.AddExperience(1);
    assert(growth.Level() == 2 && growth.Experience() == 0 && growth.PendingChoices() == 1);
    assert(growth.Choices().size() == 3);
    assert(std::set<UpgradeId>(growth.Choices().begin(), growth.Choices().end()).size() == 3);
    const auto offered = growth.Choices();
    growth.AddExperience(1);
    assert(growth.Choices() == offered);
    assert(!growth.Select(3, reward));
    assert(growth.Select(0, reward) && growth.Rank(reward) == 1 && !growth.IsChoosing());

    // Multiple level-ups must retain overflow XP and require one selection per level.
    SurvivorProgression burst(42);
    burst.AddExperience(24); // thresholds: 5 + 8 + 11
    assert(burst.Level() == 4 && burst.Experience() == 0 && burst.PendingChoices() == 3);
    for (int pending = 3; pending > 0; --pending) {
        assert(burst.Select(0, reward));
        assert(burst.PendingChoices() == pending - 1);
    }
    assert(burst.Choices().empty());

    // Presents never consume pending level rewards; exhausted upgrades are excluded.
    SurvivorProgression chest(17);
    chest.AddExperience(5);
    for (int i = 0; i < 200; ++i) {
        const auto item = chest.GrantPresent();
        assert(chest.PendingChoices() == 1 && chest.Level() == 2);
        if (item != UpgradeId::Heal) assert(chest.Rank(item) <= GetUpgradeDefinition(item).maxRank);
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
    assert(GetUpgradeEffectText(UpgradeId::BulletHoming, 1).find("24") != std::string::npos);
    assert(GetUpgradeEffectText(UpgradeId::BulletHoming, 5).find("120") != std::string::npos);
    assert(GetUpgradeEffectText(UpgradeId::RapidFire, 1).find("0.085") != std::string::npos);
    assert(chest.Choices().size() == 1 && chest.Choices()[0] == UpgradeId::Heal);
    assert(chest.Select(0, reward) && reward == UpgradeId::Heal);

    // Fractional XP bonuses must accumulate even when each kill awards only one XP.
    SurvivorProgression bonus(91);
    while (bonus.Rank(UpgradeId::Experience) == 0) bonus.GrantPresent();
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
