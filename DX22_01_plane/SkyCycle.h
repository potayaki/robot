/// @file SkyCycle.h
/// @brief 朝・昼・夕方・夜の進行と、朝昼の曇り抽選。

#pragma once

#include <algorithm>
#include <cmath>

// 空の時間帯と天候。描画から分離し、実測秒数だけで進行させる。
namespace SkyWeather {
    enum class Preset {
        Morning,
        Day,
        Evening,
        Night,
        Overcast,
        Count
    };

    class Cycle {
    public:
        // 初期状態は昼。1周6分、朝と昼の開始時にそれぞれ10%で曇りにする。
        /// 朝・昼・夕方・夜を一周する時間（秒）。
        float m_durationSeconds = 360.0f;
        /// 朝と昼の開始時に曇りを選ぶ確率（0～1）。
        float m_cloudyProbability = 0.10f;

        Preset TimeOfDay() const {
            return static_cast<Preset>(m_phase);
        }

        Preset Current() const {
            return m_cloudy ? Preset::Overcast : TimeOfDay();
        }

        float Progress() const {
            return static_cast<float>(m_progress);
        }

        template <class Random> void RollWeather(Random&& random) {
            // 抽選は時間帯の切替時だけ行う。毎フレーム抽選すると曇りが頻発してしまう。
            m_cloudy = false;
            if (m_phase == 0 || m_phase == 1) {
                m_cloudy = random() < std::clamp(m_cloudyProbability, 0.0f, 1.0f);
            }
        }

        template <class Random> void Next(Random&& random) {
            m_phase = (m_phase + 1) % 4;
            m_progress = 0.0;
            RollWeather(random);
        }

        template <class Random> void Update(float deltaSeconds, Random&& random) {
            if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) {
                return;
            }
            if (!std::isfinite(m_durationSeconds)) {
                return;
            }

            // 進み具合を割合で保持するので、Debugで1周の長さを変えても空が飛ばない。
            m_progress += static_cast<double>(deltaSeconds) * 4.0 / (std::max)(m_durationSeconds, 60.0f);
            while (m_progress >= 1.0) {
                m_progress -= 1.0;
                m_phase = (m_phase + 1) % 4;
                RollWeather(random);
            }
        }

    private:
        /// 朝・昼・夕方・夜の現在の時間帯番号（0～3）。
        int m_phase = 1;
        /// 現在の時間帯内の進行率（0～1）。
        double m_progress = 0.0;
        /// 現在の時間帯で曇りが選ばれているか。
        bool m_cloudy = false;
    };

    inline float TransitionWeight(float seconds) {
        // 切替の最初と最後を緩やかにして、空と照明を8秒かけてなじませる。
        const float t = std::clamp(seconds / 8.0f, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
} // namespace SkyWeather
