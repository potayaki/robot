/// @file EnemyRarityTests.cpp
/// @brief 亜種・希少種の能力、端数処理、色、抽選確率を検証する。

#include "../DX22_01_plane/EnemyRarity.h"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace EnemyVariants;
    for (const int base : {1, 2, 3, 4, 6, 8, 12, 250}) {
        assert(Health(base, EnemyRarity::Normal) == base);
        assert(Damage(base, EnemyRarity::Normal) == base);
        assert(Health(base, EnemyRarity::Variant) > base);
        assert(Health(base, EnemyRarity::Rare) > Health(base, EnemyRarity::Variant));
        assert(Damage(base, EnemyRarity::Rare) >= Damage(base, EnemyRarity::Variant));
        assert(OrbDropWeight(base, EnemyRarity::Rare) > OrbDropWeight(base, EnemyRarity::Variant));
    }
    assert(Health(3, EnemyRarity::Variant) == 6 && Health(3, EnemyRarity::Rare) == 12);
    assert(Damage(3, EnemyRarity::Variant) == 5); // 1.5倍の端数を失わない。
    assert(Damage(0, EnemyRarity::Rare) == 0 && Damage(-3, EnemyRarity::Rare) == 0);
    assert(OrbDropWeight(-1, EnemyRarity::Rare) == 0);
    // 敵本来の抽選率によらず亜種・希少種は固定し、通常種は本来の抽選率を使う。
    for (const int base : {-40, 0, 10, 12, 25, 95, 100, 120}) {
        assert(PresentChance(base, EnemyRarity::Variant) == 5);
        assert(PresentChance(base, EnemyRarity::Rare) == 20);
        assert(PresentChance(base, EnemyRarity::Normal) == std::clamp(base, 0, 100));
    }
    assert(Normalize(static_cast<EnemyRarity>(-1)) == EnemyRarity::Normal);
    assert(Normalize(static_cast<EnemyRarity>(3)) == EnemyRarity::Normal);
    for (const auto species : {EnemySpecies::Slime, EnemySpecies::Fly, EnemySpecies::Boar,
                               EnemySpecies::Mushroom, EnemySpecies::Ghost}) {
        assert(Tint(species, EnemyRarity::Normal).w == 0);
        const auto variant = Tint(species, EnemyRarity::Variant);
        const auto rare = Tint(species, EnemyRarity::Rare);
        assert(variant != rare && variant.w > 0 && rare.w > 0);
        for (const auto color : {variant, rare}) {
            assert(color.x >= 0 && color.x <= 1 && color.y >= 0 && color.y <= 1);
            assert(color.z >= 0 && color.z <= 1 && color.w <= 1);
        }
    }
    for (const auto chances : {std::array{20, 3}, std::array{0, 0}, std::array{60, 20}, std::array{100, 100},
                               std::array{-20, -3}}) {
        int counts[3]{};
        for (int roll = 0; roll < 100; ++roll) {
            ++counts[static_cast<int>(Choose(roll, chances[0], chances[1]))];
        }
        const int rare = std::clamp(chances[1], 0, 100);
        const int variant = std::clamp(chances[0], 0, 100 - rare);
        assert(counts[2] == rare && counts[1] == variant && counts[0] == 100 - rare - variant);
    }
    std::cout << "Enemy rarity stats, rewards, palettes and exact spawn distributions passed.\n";
}
