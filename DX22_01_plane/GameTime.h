/// @file GameTime.h
/// @brief 実測時間の分割、ゲーム停止、秒単位のタイマー更新。

#pragma once
#include <algorithm>
#include <cmath>

// Applicationで実測した秒数を、ゲーム全体で共有する。
// 描画FPSの上限とゲーム内の時間を分離し、30fpsでも120fpsでも同じ速さで進める。
class GameTime {
private:
    /// まだ更新処理へ渡していない実測の経過秒数。
    static inline double m_remainingSeconds = 0.0;
    /// 今回のゲーム更新に使う経過秒数。一時停止中は進めない。
    static inline float m_deltaSeconds = 0.0f;
    /// 一時停止の影響を受けない実測の経過秒数。
    static inline float m_unscaledDeltaSeconds = 0.0f;

public:
    static void BeginFrame(double elapsedSeconds, float timeScale, bool paused) {
        if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0) {
            elapsedSeconds = 0.0;
        }
        m_unscaledDeltaSeconds = static_cast<float>(elapsedSeconds);
        m_deltaSeconds = 0.0f;
        m_remainingSeconds = paused ? 0.0 : elapsedSeconds * (std::max)(0.0f, timeScale);
    }

    // 長いフレームは小分けにして衝突や移動を更新する。
    // 経過時間は切り捨てず、最後の端数も使う。固定フレーム数を数える処理ではない。
    static bool NextStep() {
        constexpr double maxStepSeconds = 1.0 / 30.0;
        const double step = (std::min)(m_remainingSeconds, maxStepSeconds);
        m_remainingSeconds -= step;
        m_deltaSeconds = static_cast<float>(step);
        return step > 0.0;
    }

    // 移動・寿命・攻撃など、ゲームを進める処理で使う秒数。
    static float DeltaSeconds() {
        return m_deltaSeconds;
    }

    // 一時停止中も操作するUIやコントローラーの振動で使う、実際のフレーム間隔。
    static float UnscaledDeltaSeconds() {
        return m_unscaledDeltaSeconds;
    }

    // 残り時間を0で止める。寿命・無敵時間・UIの入力待ちなどに使用する。
    static void CountDown(float& remainingSeconds, float elapsedSeconds) {
        remainingSeconds = (std::max)(0.0f, remainingSeconds - elapsedSeconds);
    }

    // 連射のタイマーは0を越えた端数を残し、次の発射時に間隔を += する。
    // 毎回0へ丸めると、端数のある発射間隔がFPSによって長くなるため。
    static void AdvanceCooldown(float& remainingSeconds, float elapsedSeconds) {
        if (remainingSeconds > 0.0f) {
            remainingSeconds -= elapsedSeconds;
        }
    }
};
