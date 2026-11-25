/// @brief 敵を倒した地点から散る経験値の粒の個数・価値・初速を決める。
#pragma once
#include "ExperienceOrb.h"
#include <random>
#include <vector>

inline void AppendExperienceOrbDrop(std::vector<ExperienceOrb>& orbs,
                                    const DirectX::SimpleMath::Vector3& origin, float groundY, int dropWeight,
                                    std::mt19937& random) {
    if (dropWeight <= 0) {
        return;
    }
    // 8粒・9粒の重みは各1。強い敵ほど10粒が出やすく、各粒の価値は同じ範囲で抽選する。
    std::discrete_distribution<int> extraOrbs({1.0, 1.0, static_cast<double>(dropWeight)});
    const int count = 8 + extraOrbs(random);
    std::uniform_int_distribution<int> experience(2, 5);
    std::uniform_real_distribution<float> turn(0.0f, DirectX::XM_2PI);
    std::uniform_real_distribution<float> sectorJitter(-0.22f, 0.22f);
    std::uniform_real_distribution<float> outwardSpeed(24.0f, 50.0f);
    std::uniform_real_distribution<float> upwardSpeed(35.0f, 56.0f);
    const float rotation = turn(random);
    const float sector = DirectX::XM_2PI / count;
    for (int i = 0; i < count; ++i) {
        // 方角を分けてから少しずらし、粒が重ならず不揃いな輪に散るようにする。
        const float angle = rotation + sector * (i + sectorJitter(random));
        const float speed = outwardSpeed(random);
        const DirectX::SimpleMath::Vector3 velocity(std::cos(angle) * speed, upwardSpeed(random),
                                                    std::sin(angle) * speed);
        orbs.emplace_back(origin, groundY, experience(random), angle, velocity);
    }
}
