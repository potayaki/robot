/// @file EnemyCrowdTests.cpp
/// @brief 群れの接近、半径に応じた分離、停止、各FPSでの密集状態を検証する。

#include "../DX22_01_plane/EnemyCrowd.h"
#include "../DX22_01_plane/ForestLayout.h"
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
#include <cstdlib>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

using DirectX::SimpleMath::Vector3;
using EnemyCrowd::Neighbor;

namespace {
    float FlatDistance(const Vector3& first, const Vector3& second) {
        Vector3 difference = first - second;
        difference.y = 0;
        return difference.Length();
    }

    void CheckCrowd(int fps) {
        constexpr size_t count = 90;
        std::vector<Neighbor> crowd;
        std::array<float, count> speeds{};
        for (size_t i = 0; i < count; ++i) {
            const float angle = static_cast<float>(i) * 2.39996323f;
            const float distance = 220.0f + static_cast<float>(i) * 2.3f;
            const float radius = i % 9 == 0 ? 18.0f : i % 4 == 0 ? 7.0f : 10.0f;
            const float raritySpeed = i % 7 == 0 ? 1.22f : i % 3 == 0 ? 1.12f : 1.0f;
            speeds[i] = (radius == 18 ? 18.0f : radius == 7 ? 57.0f : 30.0f) * raritySpeed;
            crowd.push_back(
                {Vector3(std::cos(angle) * distance, radius - 5, std::sin(angle) * distance), radius, i + 1});
        }
        const Vector3 player(0, 5, 0);
        const float seconds = 1.0f / fps;
        for (int frame = 0; frame < fps * 45; ++frame) {
            for (size_t i = 0; i < count; ++i) {
                const auto previous = crowd[i].center;
                const bool dash = crowd[i].radius == 7 && std::fmod((frame + 1) * seconds, 2.5f) < 25.0f / 60;
                const float speed = speeds[i] * (dash ? 1.7f : 1.0f);
                crowd[i].center = EnemyCrowd::Move(previous, player, crowd[i].radius, 10, speed, seconds,
                                                   crowd, crowd[i].id);
                assert(std::isfinite(crowd[i].center.x) && std::isfinite(crowd[i].center.z));
                assert(crowd[i].center.y == previous.y);
                assert(FlatDistance(previous, crowd[i].center) <= speed * seconds + 0.002f);
                assert(FlatDistance(player, crowd[i].center) >=
                       (crowd[i].radius + 10) * EnemyCrowd::ContactRatio - 0.01f);
            }
        }
        size_t outsideFrontRow = 0;
        float minimumRatio = 1000;
        float averageDistance = 0;
        for (size_t i = 0; i < count; ++i) {
            const float distance = FlatDistance(player, crowd[i].center);
            outsideFrontRow += distance > 35;
            averageDistance += distance / count;
            for (size_t j = i + 1; j < count; ++j) {
                minimumRatio = (std::min)(minimumRatio, FlatDistance(crowd[i].center, crowd[j].center) /
                                                            (crowd[i].radius + crowd[j].radius));
            }
        }
        std::cout << fps << " FPS / 90 slimes / 45 seconds: outer=" << outsideFrontRow
                  << ", mean radius=" << averageDistance << ", minimum body ratio=" << minimumRatio
                  << std::endl;
        assert(outsideFrontRow >= 65 && averageDistance > 60);
        assert(minimumRatio > 0.8f);
    }
} // namespace

int main() {
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#ifdef _DEBUG
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    const Vector3 player(0, 5, 0);
    const Vector3 initial(0, 5, 100);
    const std::vector<Neighbor> empty;
    assert(EnemyCrowd::Move(initial, player, 10, 10, 30, 0, empty, 1) == initial);
    assert(EnemyCrowd::Move(initial, player, 10, 10, 30, -1, empty, 1) == initial);
    assert(EnemyCrowd::Move(initial, player, 10, 10, 30, std::numeric_limits<float>::quiet_NaN(), empty, 1) ==
           initial);
    Vector3 lone = initial;
    for (int i = 0; i < 300; ++i) {
        lone = EnemyCrowd::Move(lone, player, 10, 10, 30, 1.0f / 60, empty, 1);
    }
    // 最前列は接触攻撃を続けられ、プレイヤー中心には進まない。
    assert(FlatDistance(lone, player) < 20 && FlatDistance(lone, player) > 18);
    assert(EnemyCrowd::Move(lone, player, 10, 10, 30, 1.0f / 60, empty, 1) == lone);

    std::vector<Neighbor> overlapped{{Vector3(100, 7, 100), 7, 1}, {Vector3(100, 18, 100), 18, 2}};
    const Vector3 distantPlayer(0, 5, -1000);
    for (int frame = 0; frame < 120; ++frame) {
        for (auto& member : overlapped) {
            const auto previous = member.center;
            member.center = EnemyCrowd::Move(previous, distantPlayer, member.radius, 10, 30, 1.0f / 60,
                                             overlapped, member.id);
            assert(std::isfinite(member.center.x) && std::isfinite(member.center.z));
            assert(FlatDistance(previous, member.center) <= 0.501f);
        }
    }
    assert(FlatDistance(overlapped[0].center, overlapped[1].center) >= 24.9f);

    // 分離後の移動を既存の門・外周処理へ渡しても、閉じた門を越えない。
    const Vector3 atGate(0, 5, 829);
    const std::vector<Neighbor> behind{{Vector3(0, 5, 817), 18, 2}};
    const auto requested = EnemyCrowd::Move(atGate, Vector3(0, 5, 1100), 10, 10, 30, 1.0f / 30, behind, 1);
    const auto constrained = ForestLayout::ConstrainMovement(atGate, requested, 10, 0);
    assert(constrained.z <= 830 && ForestLayout::IsWalkable(constrained, 10, 0));

    for (int fps : {30, 60, 144}) {
        CheckCrowd(fps);
    }
}
