/// @brief 開門進行、門の掃引衝突、キャンプとボスアリーナの境界を検証する。
#include "../DX22_01_plane/ForestLayout.h"
#include "../DX22_01_plane/ForestProgress.h"
#include "../DX22_01_plane/BossBattleRules.h"
#include <cassert>
#include <cmath>
#include <iostream>

using DirectX::SimpleMath::Vector3;
using ForestLayout::Area;

namespace {
    void CheckGateProgress() {
        ForestGateProgress gate;
        assert(!gate.IsUnlocked() && !gate.IsOpen());
        for (BossKind kind : {BossKind::Fly, BossKind::Boar, BossKind::Mushroom, BossKind::Ghost}) {
            gate.OnBossDefeated(kind);
            gate.Update(10.0f);
            assert(!gate.IsUnlocked() && !gate.IsOpen() && gate.OpenAmount() == 0.0f);
        }
        gate.OnBossDefeated(BossKind::Slime);
        assert(gate.IsUnlocked() && !gate.IsOpen());
        gate.Update(ForestGateProgress::OpeningSeconds * 0.5f);
        const float halfway = gate.OpenAmount();
        assert(std::abs(halfway - 0.5f) < 0.001f);
        gate.Update(0.0f);
        gate.Update(-1.0f);
        gate.OnBossDefeated(BossKind::Slime);
        gate.OnBossDefeated(BossKind::Ghost);
        assert(gate.OpenAmount() == halfway && !gate.IsOpen());

        const Vector3 before(0, 5, ForestLayout::GateZ - 300);
        const Vector3 after(0, 200, ForestLayout::GateZ + 300);
        const auto waiting = ForestLayout::ConstrainMovement(before, after, 10, gate.IsOpen());
        assert(waiting.z == ForestLayout::GateZ - ForestLayout::GateHalfDepth - 10);
        gate.Update(ForestGateProgress::OpeningSeconds * 0.5f);
        assert(gate.IsOpen());
        assert(ForestLayout::ConstrainMovement(before, after, 10, gate.IsOpen()) == after);
        gate.Update(100.0f);
        gate.OnBossDefeated(BossKind::Slime);
        assert(gate.IsOpen() && gate.OpenAmount() == 1.0f);
        const ForestGateProgress restarted;
        assert(!restarted.IsUnlocked() && !restarted.IsOpen());
    }

    void CheckSequentialProgress() {
        ForestGateProgress progress;
        constexpr std::array bosses{BossKind::Slime, BossKind::Fly, BossKind::Boar};
        for (int gate = 0; gate < ForestLayout::GateCount; ++gate) {
            const auto area = static_cast<Area>(gate);
            const auto nextArea = static_cast<Area>(gate + 1);
            assert(progress.OpenGateCount() == gate);
            assert(progress.NextBoss(area) == bosses[gate]);
            assert(!progress.IsAreaCleared(area));

            // 同じボスでも別の部屋からの通知では門を開かない。
            progress.OnBossDefeated(bosses[gate], nextArea);
            progress.OnBossDefeated(BossKind::Ghost, area);
            progress.OnBossDefeated(progress.NextBoss(nextArea), nextArea);
            assert(!progress.IsUnlocked(gate) && !progress.IsAreaCleared(area));
            assert(!progress.IsAreaCleared(nextArea));

            progress.OnBossDefeated(bosses[gate], area);
            assert(progress.IsUnlocked(gate) && progress.IsAreaCleared(area));
            assert(!progress.IsOpen(gate) && progress.OpenGateCount() == gate);
            progress.Update(ForestGateProgress::OpeningSeconds * 0.5f);
            progress.OnBossDefeated(bosses[gate], area);
            progress.Update(0.0f);
            assert(std::abs(progress.OpenAmount(gate) - 0.5f) < 0.001f);
            // 解錠後も、門が開き切るまでは次の部屋の撃破を受け付けない。
            progress.OnBossDefeated(progress.NextBoss(nextArea), nextArea);
            assert(!progress.IsAreaCleared(nextArea));
            assert(progress.NextBoss(Area::Keep) == BossKind::Mushroom);
            progress.Update(ForestGateProgress::OpeningSeconds * 0.5f);
            assert(progress.IsOpen(gate) && progress.OpenGateCount() == gate + 1);
        }

        assert(!progress.IsComplete() && !progress.IsAreaCleared(Area::Keep));
        progress.OnBossDefeated(BossKind::Ghost, Area::Keep);
        progress.OnBossDefeated(BossKind::Mushroom, Area::Courtyard);
        assert(progress.NextBoss(Area::Keep) == BossKind::Mushroom);
        progress.OnBossDefeated(BossKind::Mushroom, Area::Keep);
        assert(progress.NextBoss(Area::Keep) == BossKind::Ghost && !progress.IsComplete());
        progress.OnBossDefeated(BossKind::Mushroom, Area::Keep);
        progress.OnBossDefeated(BossKind::Ghost, Area::Camp);
        assert(!progress.IsComplete());
        progress.OnBossDefeated(BossKind::Ghost, Area::Keep);
        assert(progress.IsComplete() && progress.IsAreaCleared(Area::Keep));
        progress.OnBossDefeated(BossKind::Mushroom, Area::Keep);
        progress.Update(100);
        assert(progress.IsComplete() && progress.OpenGateCount() == ForestLayout::GateCount);
        const ForestGateProgress restarted;
        assert(!restarted.IsComplete() && restarted.OpenGateCount() == 0);
    }

    void CheckMultipleGateMovement() {
        constexpr float radius = 10;
        const Vector3 before(0, 5, ForestLayout::GateZ - 200);
        const Vector3 beyond(0, 99, ForestLayout::KeepMaxZ + 1000);
        for (int open = 0; open <= ForestLayout::GateCount; ++open) {
            const auto reachableArea = static_cast<Area>(open);
            const auto reached = ForestLayout::ConstrainMovement(before, beyond, radius, open);
            assert(reached.z == ForestLayout::RoomMaxZ(reachableArea) - radius);
            assert(reached.y == beyond.y && ForestLayout::IsWalkable(reached, radius, open));
            assert(ForestLayout::GetArea(beyond, open) == reachableArea);
            assert(ForestLayout::ConstrainMovement(reached, before, radius, open) == before);
        }
        for (int gate = 0; gate < ForestLayout::GateCount; ++gate) {
            const float threshold = ForestLayout::GatePositions[gate] + ForestLayout::GateHalfDepth;
            assert(ForestLayout::GetArea(Vector3(0, 5, threshold - 1)) == static_cast<Area>(gate));
            assert(ForestLayout::GetArea(Vector3(0, 5, threshold)) == static_cast<Area>(gate + 1));
        }
        // 最初の門を通れる斜線でも、次の門で開口を外れる場合はそこで止める。
        const auto secondWall =
            ForestLayout::ConstrainMovement(Vector3(0, 5, 800), Vector3(500, 90, 5000), radius, 3);
        assert(secondWall.z == ForestLayout::GatePositions[1] - ForestLayout::GateHalfDepth - radius);
        const auto reverseSecondWall =
            ForestLayout::ConstrainMovement(Vector3(0, 5, 4400), Vector3(500, 90, 500), radius, 3);
        assert(reverseSecondWall.z == ForestLayout::GatePositions[1] + ForestLayout::GateHalfDepth + radius);
        assert(ForestLayout::GetArea(beyond, -1) == Area::Entrance);
        assert(ForestLayout::GetArea(beyond, 100) == Area::Keep);
    }

    void CheckGateMovement(int gate) {
        const float gateZ = ForestLayout::GatePositions[gate];
        const int openGateCount = gate + 1;
        for (float radius : {10.0f, 12.0f, 18.0f, 35.0f, 55.0f}) {
            const float front = gateZ - ForestLayout::GateHalfDepth - radius;
            const float back = gateZ + ForestLayout::GateHalfDepth + radius;
            const float opening = ForestLayout::GateHalfWidth - radius;
            for (float height : {5.0f, 100.0f, 1000.0f}) {
                const Vector3 before(0, 5, front - 200);
                const Vector3 after(0, height, back + 200);
                const auto closed = ForestLayout::ConstrainMovement(before, after, radius, gate);
                assert(closed == Vector3(0, height, front));
                assert(!ForestLayout::IsWalkable(after, radius, gate));
                assert(ForestLayout::IsWalkable(closed, radius, gate));
                assert(ForestLayout::ConstrainMovement(before, after, radius, openGateCount) == after);
                assert(ForestLayout::ConstrainMovement(after, before, radius, openGateCount) == before);

                // 大きな1フレーム移動とジャンプでも、開口の左右にある壁を越えない。
                for (float x : {-500.0f, -opening - 0.1f, opening + 0.1f, 500.0f}) {
                    const Vector3 wallBefore(x, 5, front - 200);
                    const Vector3 wallAfter(x, height, back + 1000);
                    const auto hit =
                        ForestLayout::ConstrainMovement(wallBefore, wallAfter, radius, openGateCount);
                    assert(hit == Vector3(x, height, front));
                    const auto reverse =
                        ForestLayout::ConstrainMovement(wallAfter, wallBefore, radius, openGateCount);
                    assert(reverse == Vector3(x, 5, back));
                }
                // 体全体が開口へ収まれば、その境界に沿って通過できる。
                for (float x : {-opening, opening}) {
                    const Vector3 edgeBefore(x, 5, front - 100);
                    const Vector3 edgeAfter(x, height, back + 100);
                    assert(ForestLayout::ConstrainMovement(edgeBefore, edgeAfter, radius, openGateCount) ==
                           edgeAfter);
                }
            }

            // 目的地だけは通行可能な斜め移動も、途中で壁に当たれば手前で止める。
            const Vector3 diagonalFrom(500, 5, front - 100);
            const Vector3 diagonalTo(0, 77, back + 100);
            const auto diagonal =
                ForestLayout::ConstrainMovement(diagonalFrom, diagonalTo, radius, openGateCount);
            assert(diagonal.z == front && diagonal.y == 77);
            assert(ForestLayout::IsWalkable(diagonal, radius, openGateCount));

            // 門の中から柱へ横移動したときはXを止め、逆方向へは離れられる。
            const Vector3 corridor(0, 8, gateZ);
            const Vector3 side(500, 91, gateZ);
            const auto pillar = ForestLayout::ConstrainMovement(corridor, side, radius, openGateCount);
            assert(pillar == Vector3(opening, 91, gateZ));
            assert(ForestLayout::ConstrainMovement(pillar, corridor, radius, openGateCount) == corridor);
            const Vector3 slide(opening, 91, back + 100);
            assert(ForestLayout::ConstrainMovement(pillar, slide, radius, openGateCount) == slide);

            // 外周を大きく越える要求も、丸める前の経路で門の壁に当てる。
            for (float farX : {-100000.0f, 100000.0f}) {
                const Vector3 hugeBefore(0, 5, front - 30);
                const Vector3 hugeAfter(farX, 73, back + 1000);
                const auto huge =
                    ForestLayout::ConstrainMovement(hugeBefore, hugeAfter, radius, openGateCount);
                assert(huge.z == front && huge.y == hugeAfter.y);
                assert(ForestLayout::IsWalkable(huge, radius, openGateCount));
                const Vector3 reverseBefore(0, 5, back + 30);
                const Vector3 reverseAfter(farX, 94, front - 1000);
                const auto reverseHuge =
                    ForestLayout::ConstrainMovement(reverseBefore, reverseAfter, radius, openGateCount);
                assert(reverseHuge.z == back && reverseHuge.y == reverseAfter.y);
                assert(ForestLayout::IsWalkable(reverseHuge, radius, openGateCount));
            }

            const float lastRoomEnd = ForestLayout::RoomMaxZ(static_cast<Area>(openGateCount));
            const Vector3 lastRoom(0, 5, (back + lastRoomEnd) * 0.5f);
            const auto outside =
                ForestLayout::ConstrainMovement(lastRoom, Vector3(9000, 18, 9000), radius, openGateCount);
            assert(outside == Vector3(ForestLayout::MaxX - radius, 18, lastRoomEnd - radius));
        }
    }

    void CheckRoomArenaBounds() {
        for (Area area : {Area::Entrance, Area::Camp, Area::Courtyard, Area::Keep}) {
            const float lowZ = ForestLayout::RoomMinZ(area);
            const float highZ = ForestLayout::RoomMaxZ(area);
            for (float x : {ForestLayout::MinX + 10, 0.0f, ForestLayout::MaxX - 10}) {
                for (float z : {lowZ + 10, (lowZ + highZ) * 0.5f, highZ - 10}) {
                    const auto center = ForestLayout::ConstrainRoom(Vector3(x, 5, z), 55, area);
                    for (float body : {10.0f, 30.0f, 32.0f, 35.0f, 36.0f, 40.0f, 55.0f}) {
                        for (float arena : {140.0f, 500.0f, 800.0f}) {
                            for (int i = 0; i < 16; ++i) {
                                const float angle = DirectX::XM_2PI * i / 16;
                                const Vector3 target =
                                    center + Vector3(std::cos(angle) * 5000, 80, std::sin(angle) * 5000);
                                const auto room = ForestLayout::ConstrainRoom(target, body, area);
                                const auto result = BossArena::ClampPosition(room, center, arena, body);
                                assert((ForestLayout::ConstrainRoom(result, body, area) - result).Length() <
                                       0.002f);
                                Vector3 offset = result - center;
                                offset.y = 0;
                                assert(offset.Length() <= arena - body + 0.002f);
                                assert(result.y == target.y);
                            }
                        }
                    }
                }
            }
        }
        assert(ForestLayout::PathCenter(ForestLayout::GateZ) == 0);
        assert(std::abs(ForestLayout::PathCenter(ForestLayout::GateZ - 0.001f)) < 0.01f);
        assert(std::abs(ForestLayout::PathCenter(ForestLayout::GateZ + 0.001f)) < 0.01f);
    }
} // namespace

int main() {
    CheckGateProgress();
    CheckSequentialProgress();
    CheckMultipleGateMovement();
    for (int gate = 0; gate < ForestLayout::GateCount; ++gate) {
        CheckGateMovement(gate);
    }
    CheckRoomArenaBounds();
    std::cout << "Forest gate and camp tests passed.\n";
}
