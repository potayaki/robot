/// @file GhostSupportRules.h
/// @brief 支援対象の選択、倍率、間合いの共通ルール。
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace GhostSupport {
    struct Settings {
        /// 同時に線をつなぐ最大数。0なら支援しない。
        int targetCount = 3;
        /// 支援可能な距離（ワールド単位）。
        float range = 300.0f;
        /// 支援対象の被ダメージ倍率。整数へ切り捨て、最低1。
        float incomingMultiplier = 0.7f;
        /// 支援対象の与ダメージ倍率。整数へ切り上げる。
        float outgoingMultiplier = 1.5f;
    };

    struct Candidate {
        /// 呼び出し元の敵一覧における添字。永続的には保存しない。
        size_t index;
        /// Ghostからの距離の2乗（ワールド単位）。
        float distanceSquared;
        /// 死亡・非表示・Ghost自身などを除いた有効な仲間か。
        bool eligible;
    };

    inline std::vector<size_t> Select(std::vector<Candidate> candidates, const Settings& settings,
                                      bool sourceAlive) {
        if (!sourceAlive || settings.targetCount <= 0 || settings.range <= 0) {
            return {};
        }
        std::erase_if(candidates, [&](const Candidate& c) {
            return !c.eligible || c.distanceSquared > settings.range * settings.range;
        });
        std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.distanceSquared < b.distanceSquared;
        });
        std::vector<size_t> result;
        for (const auto& c : candidates) {
            if (result.size() >= static_cast<size_t>(settings.targetCount)) {
                break;
            }
            result.push_back(c.index);
        }
        return result;
    }

    inline int Incoming(int damage, float multiplier) {
        return damage <= 0 ? 0 : (std::max)(1, static_cast<int>(std::floor(damage * multiplier)));
    }

    inline int Outgoing(int damage, float multiplier) {
        return damage <= 0 ? 0 : (std::max)(1, static_cast<int>(std::ceil(damage * multiplier)));
    }

    // 正なら接近、負なら後退。境界を越えない移動量（ワールド単位）。
    inline float Travel(float distance, float speed, float dt, float nearDistance, float farDistance) {
        const float step = (std::max)(0.0f, speed * dt);
        if (distance < nearDistance) {
            return -(std::min)(step, nearDistance - distance);
        }
        return distance > farDistance ? (std::min)(step, distance - farDistance) : 0.0f;
    }
} // namespace GhostSupport
