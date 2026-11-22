/// @brief 森と野営地で草が生える割合。地面の色と草の配置で共有する。
#pragma once
#include "ForestLayout.h"

namespace ForestMeadow {
    inline float Cover(float x, float z) {
        if (z >= ForestLayout::CampMaxZ) {
            return 0;
        }
        const float gateDistance = (std::min)(std::abs(z - ForestLayout::GateZ), ForestLayout::CampMaxZ - z);
        // 門の前は広く、草原の中は細く踏み固められた小道。
        const float approach = 1.0f - std::clamp(gateDistance / 225.0f, 0.0f, 1.0f);
        const float pathRadius = 29 + 8 * std::sin(z * 0.012f) + 4 * std::sin(z * 0.031f) + 28 * approach;
        const float pathMargin = std::abs(x - ForestLayout::PathCenter(z)) - pathRadius;
        const float road = std::clamp(pathMargin / 43.0f, 0.0f, 1.0f);
        const float westCamp = std::hypot((x + 810) * 0.65f, z - 1530);
        const float eastCamp = std::hypot((x - 880) * 0.8f, z - 2140);
        const float camp = std::clamp(((std::min)(westCamp, eastCamp) - 120) / 210.0f, 0.0f, 1.0f);
        const float wall = std::clamp((gateDistance - ForestLayout::GateHalfDepth) / 42.0f, 0.0f, 1.0f);
        const float cover = (std::min)({road, camp, wall});
        return cover * cover * (3.0f - 2.0f * cover);
    }
} // namespace ForestMeadow
