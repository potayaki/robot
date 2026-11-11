/// @file LowHealthGlitchStrength.h
/// @brief HPの割合から画面ノイズの強さを求める計算。

#pragma once
#include <algorithm>

inline float LowHealthGlitchStrength(int hp, int maxHp) {
    if (maxHp <= 0) {
        return 0;
    }
    return std::clamp(1.0f - 2.0f * static_cast<float>(hp) / maxHp, 0.0f, 1.0f);
}
