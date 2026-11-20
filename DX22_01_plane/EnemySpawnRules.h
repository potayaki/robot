/// @file EnemySpawnRules.h
/// @brief 群れの出現数、配置間隔、部屋境界の共通ルール。
#pragma once
#include "ForestLayout.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace EnemySpawning {
    /// 開始時は少数にし、生存時間60秒ごとに群れと同時出現枠を増やす。
    inline constexpr int InitialPackSize = 1;
    inline constexpr int InitialMaxAlive = 12;
    inline constexpr float InitialInterval = 2.4f;
    inline constexpr float RampStageSeconds = 60.0f;
    /// 6分以降の密度を一定に保ち、時間だけで際限なく敵を増やさない。
    inline constexpr int RampMaxStage = 6;
    inline constexpr int PackLimit = 6;
    inline constexpr int AliveLimit = 60;
    inline constexpr int AlivePerStage = 8;
    /// 1段階ごとの出現間隔の短縮幅と、その下限（秒）。
    inline constexpr float IntervalReduction = 0.15f;
    inline constexpr float MinimumInterval = 1.5f;
    /// プレイヤーから離れた出現範囲（ワールド座標の水平距離）。
    inline constexpr float SpawnRadiusMin = 450.0f;
    inline constexpr float SpawnRadiusMax = 750.0f;
    /// 大型スライムも壁からはみ出さないための最大判定半径。
    inline constexpr float LargestRadius = 18.0f;
    /// プレイヤーの目の前で突然接触しないための水平距離。
    inline constexpr float SafePlayerDistance = 400.0f;
    /// 同じ場所へ群れを重ねず、大型2体にも余白を残す水平間隔。
    inline constexpr float EnemySpacing = 40.0f;
    /// 壁際や密集地で別の出現地点を探す最大回数。
    inline constexpr int PositionAttempts = 24;
    /// 多数の支援先検索による負荷と支援の密集を抑えるGhostの同時出現上限。
    inline constexpr int MaxGhosts = 5;

    /// 生存時間と開始時設定から求める現在の出現設定。状態として保持しない。
    struct Pressure {
        int stage;
        int packSize;
        int maxAlive;
        float interval;
    };

    inline Pressure GetPressure(double elapsedSeconds, int basePack = InitialPackSize,
                                int baseCap = InitialMaxAlive, float baseInterval = InitialInterval,
                                float stageSeconds = RampStageSeconds, float minInterval = MinimumInterval,
                                float intervalReductionDivisor = 1.0f / IntervalReduction) {
        if (!std::isfinite(stageSeconds) || stageSeconds <= 0) {
            stageSeconds = RampStageSeconds;
        }
        // 大きな生存時間も整数へ変換する前に制限する。
        const double stage =
            std::isfinite(elapsedSeconds) && elapsedSeconds > 0
                ? (std::min)(elapsedSeconds / stageSeconds, static_cast<double>(RampMaxStage))
                : 0.0;
        const int step = static_cast<int>(stage);
        basePack = std::clamp(basePack, 0, PackLimit);
        baseCap = std::clamp(baseCap, 0, AliveLimit);
        if (!std::isfinite(baseInterval) || baseInterval <= 0) {
            baseInterval = InitialInterval;
        }
        if (!std::isfinite(minInterval) || minInterval <= 0) {
            minInterval = MinimumInterval;
        }
        if (!std::isfinite(intervalReductionDivisor) || intervalReductionDivisor <= 0) {
            intervalReductionDivisor = 1.0f / IntervalReduction;
        }
        const float lowerInterval = (std::min)(minInterval, baseInterval);
        return {step, basePack == 0 ? 0 : (std::min)(PackLimit, basePack + step),
                baseCap == 0 ? 0 : (std::min)(AliveLimit, baseCap + step * AlivePerStage),
                (std::max)(lowerInterval, baseInterval - step / intervalReductionDivisor)};
    }

    inline int PackSize(int requested, int living, int maximum) {
        const int remaining = (std::max)(0, maximum - (std::max)(0, living));
        return std::clamp(requested, 0, remaining);
    }

    /// 長いフレームでも群れは1回だけ出し、追いつき用の連続生成をためない。
    inline bool Advance(float& elapsed, float seconds, float interval) {
        if (!std::isfinite(seconds) || seconds <= 0 || !std::isfinite(interval) || interval <= 0) {
            return false;
        }
        elapsed += seconds;
        if (elapsed < interval) {
            return false;
        }
        elapsed = std::fmod(elapsed, interval);
        return true;
    }

    inline bool CanSpawnGhost(int living, int ghosts) {
        // 4体以上の支援先を確保し、Ghostが群れの20%を超えないようにする。
        return living > 0 && ghosts < MaxGhosts && (ghosts + 1) * 5 <= living + 1;
    }

    inline bool IsClearPosition(const DirectX::SimpleMath::Vector3& candidate,
                                const DirectX::SimpleMath::Vector3& player,
                                const std::vector<DirectX::SimpleMath::Vector3>& occupied, bool forest,
                                ForestLayout::Area area) {
        if (!std::isfinite(candidate.x) || !std::isfinite(candidate.z)) {
            return false;
        }
        if (forest && ForestLayout::ConstrainRoom(candidate, LargestRadius, area) != candidate) {
            return false;
        }
        const auto flatDistanceSquared = [](const auto& a, const auto& b) {
            const float dx = a.x - b.x;
            const float dz = a.z - b.z;
            return dx * dx + dz * dz;
        };
        if (flatDistanceSquared(candidate, player) < SafePlayerDistance * SafePlayerDistance) {
            return false;
        }
        for (const auto& position : occupied) {
            if (flatDistanceSquared(candidate, position) < EnemySpacing * EnemySpacing) {
                return false;
            }
        }
        return true;
    }
} // namespace EnemySpawning
