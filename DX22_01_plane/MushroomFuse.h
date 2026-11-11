/// @file MushroomFuse.h
/// @brief キノコの接近、予告点滅、自爆までの秒単位の進行。

#pragma once
#include <algorithm>
#include <cmath>

// 接近と予備動作を秒単位で管理する。描画処理から独立して検証できる。
class MushroomFuse {
public:
    /// 移動速度（ワールド単位/秒）。
    float m_speed = 24.0f;
    /// この距離まで近づいたら自爆の予備動作へ入る。
    float m_triggerDistance = 45.0f;
    /// 攻撃開始前に予備動作を行う時間（秒）。
    float m_windupSeconds = 1.5f;
    /// 自爆予告の色を点滅させる間隔（秒）。
    float m_flashInterval = 0.15f;

    /// 接近中に跳ねる周期（秒）。
    float m_hopPeriod = 0.7f;
    /// 跳躍の頂点での高さ。見た目と当たり判定の両方に反映する。
    float m_hopHeight = 50.0f;

    float Update(float dt, float distance) {
        if (dt <= 0 || Exploded()) {
            return 0;
        }
        float travel = 0;
        if (!m_armed) {
            const float gap = (std::max)(0.0f, distance - m_triggerDistance);
            travel = (std::min)(gap, m_speed * dt);
            m_chaseElapsed += travel / m_speed;
            if (gap > travel) {
                return travel;
            }
            dt -= travel / m_speed;
            m_armed = true;
        }
        m_elapsed += dt;
        return travel;
    }

    bool Armed() const {
        return m_armed;
    }

    bool Exploded() const {
        return m_armed && m_elapsed >= m_windupSeconds;
    }

    bool FlashRed() const {
        return m_armed && std::fmod(m_elapsed, m_flashInterval * 2) < m_flashInterval;
    }

    float HopHeight() const {
        if (m_armed) {
            return 0.0f;
        }
        // 4 * t * (1-t) は、周期の中央で1になる放物線。
        const float phase = std::fmod(m_chaseElapsed, m_hopPeriod) / m_hopPeriod;
        return 4.0f * m_hopHeight * phase * (1.0f - phase);
    }

private:
    /// 自爆の予備動作を開始しているか。
    bool m_armed = false;
    /// 現在の動作または演出を開始してからの経過秒数。
    float m_elapsed = 0;
    /// 接近中のアニメーションに使用する経過秒数。
    float m_chaseElapsed = 0;
};
