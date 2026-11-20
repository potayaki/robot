/// @file EnemyRarity.h
/// @brief 通常種・亜種・希少種の能力倍率、報酬、体色と出現抽選。

#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <cmath>

enum class EnemyRarity {
    Normal,
    Variant,
    Rare
};

enum class EnemySpecies {
    Slime,
    Fly,
    Boar,
    Mushroom,
    Ghost
};

namespace EnemyVariants {
    struct Stats {
        /// 元の種類・役割の最大HPに掛ける倍率。
        float healthMultiplier;
        /// Ghost支援を適用する前の攻撃力倍率。
        float damageMultiplier;
        /// 接近・突進などの移動速度倍率。攻撃の予告時間は短縮しない。
        float speedMultiplier;
        /// 撃破時の経験値倍率。
        int experienceMultiplier;
        /// プレゼントの固定抽選率（%）。-1の通常種だけ敵本来の抽選率を使う。
        int presentChance;
    };

    constexpr EnemyRarity Normalize(EnemyRarity rarity) {
        return rarity == EnemyRarity::Variant || rarity == EnemyRarity::Rare ? rarity : EnemyRarity::Normal;
    }

    constexpr Stats Get(EnemyRarity rarity) {
        switch (rarity) {
            case EnemyRarity::Variant:
                return {2.0f, 1.5f, 1.12f, 2, 20};
            case EnemyRarity::Rare:
                return {4.0f, 2.0f, 1.22f, 4, 100};
            default:
                return {1.0f, 1.0f, 1.0f, 1, -1};
        }
    }

    inline int Health(int base, EnemyRarity rarity) {
        return (std::max)(1, static_cast<int>(std::ceil(base * Get(rarity).healthMultiplier)));
    }

    inline int Damage(int base, EnemyRarity rarity) {
        return base <= 0 ? 0 : static_cast<int>(std::ceil(base * Get(rarity).damageMultiplier));
    }

    constexpr int Experience(int base, EnemyRarity rarity) {
        return (std::max)(0, base) * Get(rarity).experienceMultiplier;
    }

    constexpr int PresentChance(int base, EnemyRarity rarity) {
        const int fixedChance = Get(rarity).presentChance;
        return fixedChance >= 0 ? fixedChance : std::clamp(base, 0, 100);
    }

    constexpr const char8_t* Name(EnemyRarity rarity) {
        switch (rarity) {
            case EnemyRarity::Variant:
                return u8"亜種";
            case EnemyRarity::Rare:
                return u8"希少種";
            default:
                return u8"通常種";
        }
    }

    // aは透明度ではなく色の置換率。0は素材本来の色・模様をそのまま表示する。
    inline DirectX::SimpleMath::Color Tint(EnemySpecies species, EnemyRarity rarity) {
        using DirectX::SimpleMath::Color;
        if (rarity == EnemyRarity::Rare) {
            switch (species) {
                case EnemySpecies::Slime:
                    return Color(1.0f, 0.72f, 0.12f, 0.95f);
                case EnemySpecies::Fly:
                    return Color(0.85f, 0.92f, 1.0f, 0.95f);
                case EnemySpecies::Boar:
                    return Color(1.0f, 0.45f, 0.12f, 0.95f);
                case EnemySpecies::Mushroom:
                    return Color(0.35f, 1.0f, 0.82f, 0.95f);
                case EnemySpecies::Ghost:
                    return Color(1.0f, 0.78f, 0.22f, 0.95f);
            }
        }
        if (rarity == EnemyRarity::Variant) {
            switch (species) {
                case EnemySpecies::Slime:
                    return Color(0.15f, 0.55f, 1.0f, 0.88f);
                case EnemySpecies::Fly:
                    return Color(0.78f, 0.25f, 1.0f, 0.88f);
                case EnemySpecies::Boar:
                    return Color(0.27f, 0.5f, 0.9f, 0.88f);
                case EnemySpecies::Mushroom:
                    return Color(0.65f, 0.22f, 1.0f, 0.88f);
                case EnemySpecies::Ghost:
                    return Color(1.0f, 0.25f, 0.38f, 0.88f);
            }
        }
        return Color(1, 1, 1, 0);
    }

    // 抽選値は0～99。希少種と亜種を別々の枠で確保し、残りを通常種にする。
    constexpr EnemyRarity Choose(int roll, int variantPercent, int rarePercent) {
        roll = std::clamp(roll, 0, 99);
        rarePercent = std::clamp(rarePercent, 0, 100);
        variantPercent = std::clamp(variantPercent, 0, 100 - rarePercent);
        if (roll < rarePercent) {
            return EnemyRarity::Rare;
        }
        if (roll < rarePercent + variantPercent) {
            return EnemyRarity::Variant;
        }
        return EnemyRarity::Normal;
    }
} // namespace EnemyVariants
