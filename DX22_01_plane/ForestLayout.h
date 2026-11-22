/// @brief 森、キャンプ、中庭、城内の移動範囲と、門を含めた掃引衝突。
#pragma once
#include "ForestTerrain.h"
#include <SimpleMath.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace ForestLayout {
    enum class Area {
        Entrance,
        Camp,
        Courtyard,
        Keep
    };

    inline constexpr float MinX = -760.0f;
    inline constexpr float MaxX = 760.0f;
    inline constexpr float MinZ = -1050.0f;
    inline constexpr float MaxZ = 900.0f;
    inline constexpr float GateZ = MaxZ;
    inline constexpr float GateHalfWidth = 130.0f;
    inline constexpr float GateHalfDepth = 60.0f;
    inline constexpr float CampMaxZ = 2600.0f;
    inline constexpr float CourtyardMaxZ = 4250.0f;
    inline constexpr float KeepMaxZ = 5900.0f;
    /// 各部屋から次の部屋へ進む門の数とワールドZ座標。
    inline constexpr int GateCount = 3;
    inline constexpr std::array<float, GateCount> GatePositions{GateZ, CampMaxZ, CourtyardMaxZ};
    /// 入口から順に、右手の森の門、左手の砦の門、右手の城門へ向かう。
    inline constexpr std::array<float, GateCount> GateCentersX{300.0f, -310.0f, 245.0f};

    inline DirectX::SimpleMath::Vector3 Constrain(DirectX::SimpleMath::Vector3 position, float radius) {
        position.x = std::clamp(position.x, MinX + radius, MaxX - radius);
        position.z = std::clamp(position.z, MinZ + radius, MaxZ - radius);
        return position;
    }

    inline bool IsCamp(const DirectX::SimpleMath::Vector3& position) {
        return position.z >= GateZ + GateHalfDepth;
    }

    inline float RoomMinZ(Area area) {
        const size_t index = static_cast<size_t>(area);
        return index == 0 ? MinZ : GatePositions[index - 1] + GateHalfDepth;
    }

    inline float RoomMaxZ(Area area) {
        const size_t index = static_cast<size_t>(area);
        return index == GateCount ? KeepMaxZ : GatePositions[index] - GateHalfDepth;
    }

    /// 開き切った門の数を上限として、通過済みの部屋を返す。
    inline Area GetArea(const DirectX::SimpleMath::Vector3& position, int openGateCount = GateCount) {
        int area = 0;
        for (int gate = 0; gate < std::clamp(openGateCount, 0, GateCount); ++gate) {
            if (position.z < GatePositions[gate] + GateHalfDepth) {
                break;
            }
            area = gate + 1;
        }
        return static_cast<Area>(area);
    }

    /// 門を含めず、指定した部屋の内側へ体全体を収める。高さは維持する。
    inline DirectX::SimpleMath::Vector3 ConstrainRoom(DirectX::SimpleMath::Vector3 position, float radius,
                                                      Area area) {
        position.x = std::clamp(position.x, MinX + radius, MaxX - radius);
        position.z = std::clamp(position.z, RoomMinZ(area) + radius, RoomMaxZ(area) - radius);
        return position;
    }

    inline DirectX::SimpleMath::Vector3 ConstrainRoom(DirectX::SimpleMath::Vector3 position, float radius,
                                                      bool camp) {
        return ConstrainRoom(position, radius, camp ? Area::Camp : Area::Entrance);
    }

    /// 門と仕切り壁は高さによらず判定し、ジャンプでエリアを飛び越えない。
    inline bool IsWalkable(const DirectX::SimpleMath::Vector3& position, float radius, int openGateCount) {
        openGateCount = std::clamp(openGateCount, 0, GateCount);
        if (position.x < MinX + radius || position.x > MaxX - radius || position.z < MinZ + radius ||
            position.z > RoomMaxZ(static_cast<Area>(openGateCount)) - radius) {
            return false;
        }
        for (int gate = 0; gate < openGateCount; ++gate) {
            const float entranceEdge = GatePositions[gate] - GateHalfDepth - radius;
            const float exitEdge = GatePositions[gate] + GateHalfDepth + radius;
            if (position.z > entranceEdge && position.z < exitEdge &&
                std::abs(position.x - GateCentersX[gate]) > GateHalfWidth - radius) {
                return false;
            }
        }
        return true;
    }

    /// 移動線分と、体の半径分広げた壁の最初の交差を求める。
    inline void SweepGateWall(const DirectX::SimpleMath::Vector3& from,
                              const DirectX::SimpleMath::Vector3& to, float lowX, float highX, float lowZ,
                              float highZ, float& hitTime, bool& blockX, float& boundary) {
        float enter = 0.0f;
        float leave = 1.0f;
        bool enterX = false;
        float enterBoundary = 0.0f;
        for (int axis = 0; axis < 2; ++axis) {
            const float origin = axis == 0 ? from.x : from.z;
            const float delta = axis == 0 ? to.x - from.x : to.z - from.z;
            const float low = axis == 0 ? lowX : lowZ;
            const float high = axis == 0 ? highX : highZ;
            if (std::abs(delta) < 0.000001f) {
                if (origin <= low || origin >= high) {
                    return;
                }
                continue;
            }
            const float nearBoundary = delta > 0 ? low : high;
            const float nearTime = (nearBoundary - origin) / delta;
            const float farTime = ((delta > 0 ? high : low) - origin) / delta;
            if (nearTime >= enter) {
                enter = nearTime;
                enterX = axis == 0;
                enterBoundary = nearBoundary;
            }
            leave = (std::min)(leave, farTime);
        }
        if (enter < leave && enter < hitTime && leave > 0.0f) {
            hitTime = enter;
            blockX = enterX;
            boundary = enterBoundary;
        }
    }

    /// XZの移動経路を壁へ掃引し、衝突後は壁に沿って滑る。requestedのYは変更しない。
    inline DirectX::SimpleMath::Vector3 ConstrainMovement(DirectX::SimpleMath::Vector3 previous,
                                                          DirectX::SimpleMath::Vector3 requested,
                                                          float radius, int openGateCount) {
        openGateCount = std::clamp(openGateCount, 0, GateCount);
        if (openGateCount == 0) {
            return ConstrainRoom(requested, radius, Area::Entrance);
        }
        if (!IsWalkable(previous, radius, openGateCount)) {
            previous = ConstrainRoom(previous, radius, GetArea(previous, openGateCount));
        }
        previous.y = requested.y;
        // 先に外周へ丸めると移動方向が変わるため、要求された経路のまま門を判定する。
        const float sweepMinX = (std::min)(MinX - radius, requested.x);
        const float sweepMaxX = (std::max)(MaxX + radius, requested.x);
        // 2軸の衝突まで解決すれば、残りの移動は壁沿いか停止になる。
        for (int contact = 0; contact < 2; ++contact) {
            float hitTime = 1.0f;
            bool blockX = false;
            float boundary = 0.0f;
            for (int gate = 0; gate < openGateCount; ++gate) {
                const float lowZ = GatePositions[gate] - GateHalfDepth - radius;
                const float highZ = GatePositions[gate] + GateHalfDepth + radius;
                const float gateX = GateCentersX[gate];
                SweepGateWall(previous, requested, sweepMinX, gateX - GateHalfWidth + radius, lowZ, highZ,
                              hitTime, blockX, boundary);
                SweepGateWall(previous, requested, gateX + GateHalfWidth - radius, sweepMaxX, lowZ, highZ,
                              hitTime, blockX, boundary);
            }
            if (hitTime == 1.0f) {
                break;
            }
            previous += (requested - previous) * hitTime;
            if (blockX) {
                previous.x = requested.x = boundary;
            } else {
                previous.z = requested.z = boundary;
            }
        }
        requested.x = std::clamp(requested.x, MinX + radius, MaxX - radius);
        requested.z =
            std::clamp(requested.z, MinZ + radius, RoomMaxZ(static_cast<Area>(openGateCount)) - radius);
        return requested;
    }

    /// 道の中央のワールドX座標。各門の前後は開口に沿ってまっすぐ通す。
    inline float PathCenter(float z) {
        struct Waypoint {
            float z;
            float x;
        };

        constexpr float approach = GateHalfDepth + 80.0f;
        static constexpr auto route = std::to_array<Waypoint>({
            {-850.0f, -465.0f},
            {-495.0f, -520.0f},
            {-180.0f, -210.0f},
            {60.0f, 80.0f},
            {GatePositions[0] - approach, GateCentersX[0]},
            {GatePositions[0] + approach, GateCentersX[0]},
            {1770.0f, 100.0f},
            {GatePositions[1] - approach, GateCentersX[1]},
            {GatePositions[1] + approach, GateCentersX[1]},
            {GatePositions[2] - approach, GateCentersX[2]},
            {GatePositions[2] + approach, GateCentersX[2]},
            {KeepMaxZ, 0.0f},
        });
        for (size_t i = 1; i < route.size(); ++i) {
            if (z <= route[i].z) {
                const auto& from = route[i - 1];
                const auto& to = route[i];
                const float t = std::clamp((z - from.z) / (to.z - from.z), 0.0f, 1.0f);
                return std::lerp(from.x, to.x, t * t * (3.0f - 2.0f * t));
            }
        }
        return route.back().x;
    }
} // namespace ForestLayout
