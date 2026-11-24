/// @file LowHealthGlitchStrength.h
/// @brief HPの割合から画面ノイズの強さを求める計算。

#pragma once

inline float LowHealthGlitchStrength(int hp, int maxHp) {
    // 最大HPの半分で0%、残り1HPで200%になるよう、間を直線で補間する。
    constexpr float PeakStrength = 2.0f;
    const double startHp = maxHp * 0.5;
    if (maxHp <= 0 || hp >= startHp) {
        return 0;
    }
    if (hp <= 1) {
        return PeakStrength;
    }
    return static_cast<float>(PeakStrength * (startHp - hp) / (startHp - 1.0));
}
