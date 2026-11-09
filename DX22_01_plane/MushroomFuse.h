#pragma once
#include <algorithm>
#include <cmath>

// Seconds-based chase/windup logic, independent of rendering.
class MushroomFuse
{
public:
    float speed = 24.0f;
    float triggerDistance = 45.0f;
    float windupSeconds = 1.5f;
    float flashInterval = 0.15f;

    float Update(float dt, float distance)
    {
        if (dt <= 0 || Exploded()) return 0;
        float travel = 0;
        if (!m_armed)
        {
            const float gap = (std::max)(0.0f, distance - triggerDistance);
            travel = (std::min)(gap, speed * dt);
            m_chaseElapsed += travel / speed;
            if (gap > travel) return travel;
            dt -= travel / speed;
            m_armed = true;
        }
        m_elapsed += dt;
        return travel;
    }
    bool Armed() const { return m_armed; }
    bool Exploded() const { return m_armed && m_elapsed >= windupSeconds; }
    bool FlashRed() const { return m_armed && std::fmod(m_elapsed, flashInterval * 2) < flashInterval; }
    float HopHeight() const
    {
        if (m_armed) return 0.0f;
        constexpr float period = 0.7f, height = 50.0f;
        const float phase = std::fmod(m_chaseElapsed, period) / period;
        return 4.0f * height * phase * (1.0f - phase);
    }
private:
    bool m_armed = false;
    float m_elapsed = 0;
    float m_chaseElapsed = 0;
};
