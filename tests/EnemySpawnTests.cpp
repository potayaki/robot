/// @file EnemySpawnTests.cpp
/// @brief 経過時間による群れの増加、上限、部屋の四隅での遠い出現位置を検証する。
#include "../DX22_01_plane/EnemySpawnRules.h"
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

using DirectX::SimpleMath::Vector3;

int main() {
    const std::array<int, 7> expectedPacks = {3, 4, 5, 6, 6, 6, 6};
    const std::array<int, 7> expectedCaps = {12, 20, 28, 36, 44, 52, 60};
    const std::array<float, 7> expectedIntervals = {2.4f, 2.25f, 2.1f, 1.95f, 1.8f, 1.65f, 1.5f};
    for (int stage = 0; stage <= 6; ++stage) {
        const float seconds = stage * 60.0f;
        const auto pressure = EnemySpawning::GetPressure(seconds);
        assert(pressure.stage == stage);
        assert(pressure.packSize == expectedPacks[stage]);
        assert(pressure.maxAlive == expectedCaps[stage]);
        assert(std::abs(pressure.interval - expectedIntervals[stage]) < 0.0001f);
        if (stage > 0) {
            const auto before = EnemySpawning::GetPressure(std::nextafter(seconds, 0.0f));
            const auto after =
                EnemySpawning::GetPressure(std::nextafter(seconds, std::numeric_limits<float>::infinity()));
            assert(before.stage == stage - 1);
            assert(after.stage == stage);
        }
    }
    // ゲーム時間はdoubleなので、floatへの丸めで60秒より前に段階を進めない。
    assert(EnemySpawning::GetPressure(std::nextafter(60.0, 0.0)).stage == 0);
    assert(EnemySpawning::GetPressure(60.0).stage == 1);
    assert(EnemySpawning::GetPressure(std::nextafter(60.0, std::numeric_limits<double>::infinity())).stage ==
           1);

    for (float seconds : {-1.0f, -std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        const auto pressure = EnemySpawning::GetPressure(seconds);
        assert(pressure.stage == 0);
        assert(pressure.packSize == 3);
        assert(pressure.maxAlive == 12);
        assert(std::abs(pressure.interval - 2.4f) < 0.0001f);
    }
    const auto longest = EnemySpawning::GetPressure((std::numeric_limits<float>::max)());
    assert(longest.stage == 6);
    assert(longest.packSize == 6);
    assert(longest.maxAlive == 60);
    assert(std::abs(longest.interval - 1.5f) < 0.0001f);
    const auto longestDouble = EnemySpawning::GetPressure((std::numeric_limits<double>::max)());
    assert(longestDouble.stage == 6);
    assert(longestDouble.packSize == 6);
    assert(longestDouble.maxAlive == 60);
    assert(std::abs(longestDouble.interval - 1.5f) < 0.0001f);

    auto previous = EnemySpawning::GetPressure(0);
    for (int seconds = 1; seconds <= 3600; ++seconds) {
        const auto pressure = EnemySpawning::GetPressure(static_cast<float>(seconds));
        assert(pressure.stage >= previous.stage && pressure.stage <= 6);
        assert(pressure.packSize >= previous.packSize && pressure.packSize <= 6);
        assert(pressure.maxAlive >= previous.maxAlive && pressure.maxAlive <= 60);
        assert(pressure.interval <= previous.interval && pressure.interval >= 1.5f);
        previous = pressure;
    }

    // ImGuiで指定した初期値にも時間の増加を適用し、0による停止は後半も維持する。
    for (float seconds : {0.0f, 120.0f, 3600.0f}) {
        assert(EnemySpawning::GetPressure(seconds, 0).packSize == 0);
        assert(EnemySpawning::GetPressure(seconds, -1).packSize == 0);
        assert(EnemySpawning::GetPressure(seconds, 1, 0).maxAlive == 0);
        assert(EnemySpawning::GetPressure(seconds, 1, -1).maxAlive == 0);
        const auto limited = EnemySpawning::GetPressure(seconds, (std::numeric_limits<int>::max)(),
                                                        (std::numeric_limits<int>::max)());
        assert(limited.packSize == 6);
        assert(limited.maxAlive == 60);
    }
    const auto customized = EnemySpawning::GetPressure(120, 3, 20);
    assert(customized.packSize == 5);
    assert(customized.maxAlive == 36);
    const auto customTiming = EnemySpawning::GetPressure(240, 1, 12, 3.0f, 120.0f, 1.0f, 2.0f);
    assert(customTiming.stage == 2);
    assert(std::abs(customTiming.interval - 2.0f) < 0.0001f);
    const auto fasterBase = EnemySpawning::GetPressure(360, 1, 12, 1.0f);
    assert(std::abs(fasterBase.interval - 1.0f) < 0.0001f);
    for (float invalid :
         {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        const auto fallback = EnemySpawning::GetPressure(120, 1, 12, invalid, invalid, invalid, invalid);
        assert(fallback.stage == 2);
        assert(std::abs(fallback.interval - 2.1f) < 0.0001f);
    }

    assert(EnemySpawning::PackSize(6, 0, 60) == 6);
    assert(EnemySpawning::PackSize(6, 58, 60) == 2);
    assert(EnemySpawning::PackSize(6, 60, 60) == 0);
    assert(EnemySpawning::PackSize(6, 60, 12) == 0);
    assert(EnemySpawning::PackSize(-3, 0, 60) == 0);
    assert(EnemySpawning::PackSize(6, 0, 0) == 0);
    assert(EnemySpawning::PackSize(6, 0, -1) == 0);
    // 次の更新で反映される予約分も数え、同じ更新で残り2枠へ群れを重ねない。
    const int living = 58;
    int pending = 0;
    for (int pack = 0; pack < 3; ++pack) {
        pending += EnemySpawning::PackSize(6, living + pending, 60);
    }
    assert(pending == 2);

    for (int fps : {30, 60, 144}) {
        float elapsed = 0;
        float firstSpawnTime = -1;
        int count = 3; // 森の入口に最初からいるスライム。
        assert(!EnemySpawning::Advance(elapsed, 0, EnemySpawning::GetPressure(0).interval));
        for (int frame = 1; frame <= fps * 420; ++frame) {
            const float seconds = static_cast<float>(frame) / fps;
            const auto pressure = EnemySpawning::GetPressure(seconds);
            if (EnemySpawning::Advance(elapsed, 1.0f / fps, pressure.interval)) {
                if (firstSpawnTime < 0) {
                    firstSpawnTime = seconds;
                }
                const int additions = EnemySpawning::PackSize(pressure.packSize, count, pressure.maxAlive);
                assert(additions <= pressure.packSize);
                count += additions;
            }
            assert(count <= pressure.maxAlive);
            if (frame == fps * 5) {
                assert(count == 9); // 開始直後は2小隊が段階的に加わる。
            }
            if (frame % (fps * 60) == fps * 30) {
                assert(count == pressure.maxAlive); // 各段階の途中で現在の上限までゆっくり増える。
            }
        }
        assert(firstSpawnTime >= 2.4f - 0.0001f);
        assert(firstSpawnTime <= 2.4f + 1.0f / fps + 0.0001f);
        assert(count == 60);
    }

    float elapsed = 0;
    assert(!EnemySpawning::Advance(elapsed, 0, 2.4f));
    assert(!EnemySpawning::Advance(elapsed, -1, 2.4f));
    assert(!EnemySpawning::Advance(elapsed, std::numeric_limits<float>::infinity(), 2.4f));
    assert(!EnemySpawning::Advance(elapsed, std::numeric_limits<float>::quiet_NaN(), 2.4f));
    assert(!EnemySpawning::Advance(elapsed, 1, 0));
    assert(!EnemySpawning::Advance(elapsed, 1, std::numeric_limits<float>::infinity()));
    assert(elapsed == 0);
    assert(!EnemySpawning::Advance(elapsed, 1, 2.4f));
    for (int pausedFrame = 0; pausedFrame < 1800; ++pausedFrame) {
        assert(!EnemySpawning::Advance(elapsed, 0, 2.4f));
    }
    assert(elapsed == 1);
    assert(!EnemySpawning::Advance(elapsed, 1.3f, 2.4f));
    assert(EnemySpawning::Advance(elapsed, 0.11f, 2.4f));
    elapsed = 0;
    assert(EnemySpawning::Advance(elapsed, 10, 2.4f));
    assert(elapsed >= 0 && elapsed < 2.4f);
    assert(!EnemySpawning::Advance(elapsed, 0.01f, 2.4f)); // 遅延分の連続生成はしない。

    assert(!EnemySpawning::CanSpawnGhost(0, 0));
    assert(!EnemySpawning::CanSpawnGhost(3, 0));
    assert(EnemySpawning::CanSpawnGhost(4, 0));
    assert(!EnemySpawning::CanSpawnGhost(5, 1));
    assert(EnemySpawning::CanSpawnGhost(9, 1));
    assert(EnemySpawning::CanSpawnGhost(59, 4));
    assert(!EnemySpawning::CanSpawnGhost(59, 5));

    using Area = ForestLayout::Area;
    std::mt19937 random(20261120);
    std::uniform_real_distribution<float> angle(0, DirectX::XM_2PI);
    std::uniform_real_distribution<float> radius(EnemySpawning::SpawnRadiusMin,
                                                 EnemySpawning::SpawnRadiusMax);
    for (Area area : {Area::Entrance, Area::Camp, Area::Courtyard, Area::Keep}) {
        const float minZ = ForestLayout::RoomMinZ(area) + 10;
        const float maxZ = ForestLayout::RoomMaxZ(area) - 10;
        for (const auto& player :
             {Vector3(0, 5, (minZ + maxZ) * 0.5f), Vector3(ForestLayout::MinX + 10, 100, minZ),
              Vector3(ForestLayout::MaxX - 10, 5, minZ), Vector3(ForestLayout::MinX + 10, 5, maxZ),
              Vector3(ForestLayout::MaxX - 10, 100, maxZ)}) {
            std::vector<Vector3> pack;
            // 部屋の四隅でも同じ方向から6体を配置し、壁を越えたり重なったりしない。
            for (int attempt = 0; attempt < 100 && pack.size() < 6; ++attempt) {
                const float direction = angle(random);
                const Vector3 forward(std::cos(direction), 0, std::sin(direction));
                const Vector3 center = player + forward * radius(random);
                pack = EnemySpawning::PlacePack(center, forward, 6, player, {}, true, area);
            }
            assert(pack.size() == 6);
            std::vector<Vector3> occupied;
            for (const auto& member : pack) {
                assert(EnemySpawning::IsClearPosition(member, player, occupied, true, area));
                assert((member - pack.front()).Length() <= 60);
                occupied.push_back(member);
            }
            const auto limited =
                EnemySpawning::PlacePack(pack.front(), Vector3::UnitZ, 2, player, {}, true, area);
            assert(limited.size() <= 2);
            const auto blocked =
                EnemySpawning::PlacePack(pack.front(), Vector3::UnitZ, 6, player, limited, true, area);
            for (const auto& member : blocked) {
                assert(EnemySpawning::IsClearPosition(member, player, limited, true, area));
            }
            assert(!EnemySpawning::IsClearPosition(pack[0], player, pack, true, area));
            assert(!EnemySpawning::IsClearPosition(player + Vector3(10, 500, 0), player, {}, true, area));
            const Vector3 outside(ForestLayout::MaxX + 1, 5, player.z);
            assert(!EnemySpawning::IsClearPosition(outside, player, {}, true, area));
            // 同じXでも次の部屋へ飛び越えた場所には生成しない。
            const Vector3 pastGate(0, 5, ForestLayout::RoomMaxZ(area) + 100);
            assert(!EnemySpawning::IsClearPosition(pastGate, player, {}, true, area));
        }
    }
    // 高さが違っても真上に重ねない。飛行型を含む群れも水平に間隔を取る。
    assert(!EnemySpawning::IsClearPosition(Vector3(500, 100, 0), Vector3::Zero, {Vector3(500, -5, 0)}, false,
                                           Area::Entrance));
    assert(!EnemySpawning::IsClearPosition(Vector3(219, 100, 0), Vector3::Zero, {}, false, Area::Entrance));
    assert(EnemySpawning::IsClearPosition(Vector3(220, 100, 0), Vector3::Zero, {}, false, Area::Entrance));
    assert(!EnemySpawning::IsClearPosition(Vector3(std::numeric_limits<float>::quiet_NaN(), 0, 200),
                                           Vector3::Zero, {}, false, Area::Entrance));
    std::cout << "Gradual spawn pressure, pack limits, timing and four-area distant placement passed.\n";
}
