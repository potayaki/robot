/// @file GameTimeTests.cpp
/// @brief 動作と境界条件を検証する GameTimeTests の自動テスト。

#include "../DX22_01_plane/GameTime.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

namespace {
    struct Result {
        /// 時間更新テストで積算した経過秒数。
        double elapsed = 0.0;
        /// 寿命テストで計測する残り秒数。
        float remaining = 2.0f;
        /// 寿命が尽きた瞬間の実測経過秒数。
        double expiredAt = 0.0;
        /// 移動テストで積算した距離。
        float distance = 0.0f;
        /// 射撃テストで次の発射までに残っている秒数。
        float cooldown = 0.085f;
        /// 一定時間内に発射した回数。
        int shots = 0;
    };

    Result Simulate(const std::vector<double>& frames, float scale = 1.0f) {
        Result result;
        for (double frame : frames) {
            GameTime::BeginFrame(frame, scale, false);
            while (GameTime::NextStep()) {
                const float dt = GameTime::DeltaSeconds();
                assert(dt > 0.0f && dt <= 1.0f / 30.0f);
                result.elapsed += dt;

                // 2秒間の飛翔距離は、更新回数によらず1500×2=3000となる。
                const float flightTime = (std::min)(dt, result.remaining);
                result.distance += 1500.0f * flightTime;
                GameTime::CountDown(result.remaining, dt);
                if (result.remaining == 0.0f && result.expiredAt == 0.0) {
                    result.expiredAt = result.elapsed;
                }

                // 0.085秒というフレームに割り切れない間隔でも端数を失わない。
                GameTime::AdvanceCooldown(result.cooldown, dt);
                if (result.cooldown <= 0.0f) {
                    ++result.shots;
                    result.cooldown += 0.085f;
                }
            }
        }
        return result;
    }
} // namespace

int main() {
    for (int fps : {15, 30, 60, 120, 144}) {
        const auto result = Simulate(std::vector<double>(fps * 20, 1.0 / fps));
        assert(std::abs(result.elapsed - 20.0) < 0.0001);
        assert(result.remaining == 0.0f);
        assert(result.expiredAt >= 2.0 - 0.0001);
        assert(result.expiredAt <= 2.0 + 1.0 / fps + 0.0001);
        assert(std::abs(result.distance - 3000.0f) < 0.05f);
        assert(result.shots == 235);
    }

    // 可変FPSと一時的な処理落ち。0.4秒のフレームも切り捨てない。
    std::vector<double> irregular;
    for (int i = 0; i < 20; ++i) {
        for (double dt : {0.01, 0.07, 0.4, 0.02, 0.5}) {
            irregular.push_back(dt);
        }
    }
    const auto jitter = Simulate(irregular);
    assert(std::abs(jitter.elapsed - 20.0) < 0.0001);
    assert(jitter.shots == 235);
    assert(std::abs(jitter.distance - 3000.0f) < 0.05f);

    // 一時停止してもUI用の実時間は取得でき、ゲームの残り時間は減らない。
    float life = 2.0f;
    float selectionDelay = 0.2f;
    GameTime::BeginFrame(5.0, 1.0f, true);
    assert(!GameTime::NextStep());
    GameTime::CountDown(life, GameTime::DeltaSeconds());
    GameTime::CountDown(selectionDelay, GameTime::UnscaledDeltaSeconds());
    assert(life == 2.0f && selectionDelay == 0.0f);

    GameTime::BeginFrame(0.01, 1.0f, false);
    assert(GameTime::NextStep());
    GameTime::CountDown(life, GameTime::DeltaSeconds());
    assert(std::abs(life - 1.99f) < 0.00001f); // 停止中の5秒は持ち越さない。
    assert(!GameTime::NextStep());

    // 更新途中のレベルアップで中断しても、次フレームに残時間を繰り越さない。
    GameTime::BeginFrame(0.5, 1.0f, false);
    assert(GameTime::NextStep());
    GameTime::BeginFrame(0.01, 1.0f, true);
    assert(!GameTime::NextStep());
    GameTime::BeginFrame(0.01, 1.0f, false);
    assert(GameTime::NextStep());
    assert(std::abs(GameTime::DeltaSeconds() - 0.01f) < 0.00001f);
    assert(!GameTime::NextStep());

    const auto slow = Simulate(std::vector<double>(600, 1.0 / 60.0), 0.2f);
    assert(std::abs(slow.elapsed - 2.0) < 0.0001);
    assert(std::abs(slow.distance - 3000.0f) < 0.05f);

    for (double invalid : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN()}) {
        GameTime::BeginFrame(invalid, 1.0f, false);
        assert(!GameTime::NextStep());
    }
    std::cout << "Game time tests passed: FPS, jitter, lifetime, cooldown, pause, slow motion.\n";
}
