/// @brief ボスの予告時間と攻撃回数がFPSやフレーム境界で変わらないことを検証。
#include "../DX22_01_plane/BossAttackRules.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

struct Timeline {
    BossAttackPhase phase;
    float remaining;
    int attacks;
    std::array<float, 4> durations;
};

Timeline Simulate(BossKind kind, float frameSeconds) {
    BossAttackCycle cycle;
    cycle.Reset(kind);
    Timeline result{};
    float elapsed = 0;
    constexpr float total = 27.123f;
    while (elapsed < total) {
        float pending = (std::min)(frameSeconds, total - elapsed);
        elapsed += pending;
        while (pending > 0.000001f) {
            const auto phase = cycle.Phase();
            const float step = cycle.Consume(pending);
            pending -= step;
            result.durations[static_cast<int>(phase)] += step;
            if (cycle.Advance() && cycle.Phase() == BossAttackPhase::Charge) {
                ++result.attacks;
            }
        }
    }
    result.phase = cycle.Phase();
    result.remaining = cycle.Remaining();
    return result;
}

int main() {
    for (int i = 0; i < BossTypeCount; ++i) {
        const auto kind = static_cast<BossKind>(i);
        const auto reference = Simulate(kind, 1.0f / 60);
        for (float seconds : {1.0f / 15, 1.0f / 30, 1.0f / 120, 1.0f / 144, 0.137f, 7.0f}) {
            const auto result = Simulate(kind, seconds);
            assert(result.phase == reference.phase);
            assert(std::abs(result.remaining - reference.remaining) < 0.002f);
            assert(result.attacks == reference.attacks);
            for (size_t phase = 0; phase < result.durations.size(); ++phase) {
                assert(std::abs(result.durations[phase] - reference.durations[phase]) < 0.002f);
            }
        }
        BossAttackCycle cycle;
        cycle.Reset(kind);
        const float remaining = cycle.Remaining();
        assert(cycle.Consume(0) == 0);
        assert(cycle.Consume(-1) == 0);
        assert(cycle.Remaining() == remaining);
        assert(!cycle.Advance());
        cycle.Consume(remaining);
        assert(cycle.Advance());
        assert(cycle.Phase() == BossAttackPhase::Windup);
        cycle.Consume(cycle.Remaining() - 0.01f);
        assert(!cycle.Advance()); // 予告時間を使い切るまで攻撃を始めない。
    }
    BossAttackCycle boar;
    boar.Reset(BossKind::Boar);
    const BossAttackPhase sequence[] = {BossAttackPhase::Windup,  BossAttackPhase::Charge,
                                        BossAttackPhase::Windup,  BossAttackPhase::Charge,
                                        BossAttackPhase::Recover, BossAttackPhase::Approach};
    for (const auto expected : sequence) {
        boar.Consume(boar.Remaining());
        assert(boar.Advance());
        assert(boar.Phase() == expected);
        assert(boar.Progress() == 0);
    }
    std::cout << "Five boss attack timelines and double-charge warnings passed.\n";
}
