/// @file BossBattleTests.cpp
/// @brief 動作と境界条件を検証する BossBattleTests の自動テスト。

#include "../DX22_01_plane/BossBattleRules.h"
#include "../DX22_01_plane/BossCatalog.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace DirectX::SimpleMath;

int main() {
    BossRoster roster;
    assert(roster.Next() == BossKind::Slime);
    assert(roster.DefeatedCount() == 0);
    for (int index = 0; index < BossTypeCount; ++index) {
        const auto kind = static_cast<BossKind>(index);
        assert(roster.Next() == kind);
        roster.RecordDefeat(kind);
        assert(roster.DefeatedCount() == index + 1);
        assert(BossHeight(kind) >= BossRadius(kind));
        assert(BossName(kind)[0] != u8'\0');
    }
    assert(roster.Next() == BossKind::Slime);
    roster.RecordDefeat(BossKind::Slime);
    assert(roster.DefeatedCount() == BossTypeCount);
    assert(roster.Next() == BossKind::Fly);
    BossRoster retry;
    retry.SelectNext(BossKind::Ghost);
    assert(retry.DefeatedCount() == 0); // Selecting or cancelling an encounter awards no defeat.
    retry.RecordDefeat(BossKind::Ghost);
    assert(retry.Next() == BossKind::Slime);
    assert(retry.DefeatedCount() == 1);

    BossBattleProgress progress;
    assert(progress.RemainingKills(0) == 30);
    assert(!progress.ShouldStart(29));
    assert(progress.ShouldStart(30));
    assert(progress.ShouldStart(34)); // Several enemies may fall to one explosion.
    progress.Begin();
    assert(progress.IsActive());
    assert(!progress.ShouldStart(100)); // Never spawn a second boss during the duel.
    progress.Finish(35); // Includes the defeated boss, which must not count toward the next encounter.
    assert(!progress.IsActive());
    assert(progress.RemainingKills(35) == 30);
    assert(!progress.ShouldStart(64));
    assert(progress.ShouldStart(65));

    progress.m_killsPerBoss = 3;
    assert(progress.RemainingKills(36) == 2);
    assert(progress.ShouldStart(38));
    progress.Begin();
    progress.Finish(38); // Cancel on player death; restart counting after recovery.
    assert(!progress.ShouldStart(38));
    assert(progress.ShouldStart(41));

    const Vector3 center(120, -5, -500);
    const Vector3 inside(140, 25, -480);
    assert(BossArena::ClampPosition(inside, center, 220, 10) == inside);
    const auto boundary = BossArena::ClampPosition(Vector3(1120, 70, -500), center, 220, 10);
    assert(std::abs(boundary.x - 330) < 0.001f);
    assert(boundary.y == 70); // Jump height is preserved.
    assert(boundary.z == -500);
    const auto bossBoundary = BossArena::ClampPosition(Vector3(1120, 30, -500), center, 220, 35);
    assert(std::abs(bossBoundary.x - 305) < 0.001f);
    for (int index = 0; index < BossTypeCount; ++index) {
        const auto kind = static_cast<BossKind>(index);
        const auto boundary =
            BossArena::ClampPosition(Vector3(1120, BossHeight(kind), -500), center, 140, BossRadius(kind));
        assert(std::abs(boundary.x - (center.x + 140 - BossRadius(kind))) < 0.001f);
        assert(boundary.y == BossHeight(kind));
    }

    // Huge steps, diagonal travel, and every wall direction must remain inside the same fixed circle.
    for (int degrees = 0; degrees < 360; ++degrees) {
        const float angle = DirectX::XMConvertToRadians(static_cast<float>(degrees));
        const Vector3 outside = center + Vector3(std::sin(angle) * 10000, 27, std::cos(angle) * 10000);
        const auto result = BossArena::ClampPosition(outside, center, 220, 10);
        Vector3 offset = result - center;
        offset.y = 0;
        assert(std::abs(offset.Length() - 210) < 0.002f);
        assert(result.y == outside.y);
        const auto repeated = BossArena::ClampPosition(result, center, 220, 10);
        assert((result - repeated).Length() < 0.001f);
    }
    const auto oversized = BossArena::ClampPosition(inside, center, 5, 10);
    assert(oversized.x == center.x && oversized.z == center.z && oversized.y == inside.y);
    std::cout << "Boss encounter progression and arena boundary tests passed.\n";
}
