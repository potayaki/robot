#pragma once

#include <SimpleMath.h>
#include <algorithm>
#include <cmath>

// 突進の進行だけを管理する。表示・ダメージ・報酬から分離して、秒数と方向を検証できる。
class BoarCharge
{
public:
    enum class State { Chase, Windup, Charging, Recovery };
    struct Movement
    {
        bool charged = false;
        DirectX::SimpleMath::Vector3 start{}, end{};
    };

    float walkSpeed = 34.0f;
    float chargeSpeed = 210.0f;
    float chargeDistance = 240.0f;
    float windupSeconds = 1.0f;
    float recoverySeconds = 0.8f;
    float cooldownSeconds = 1.4f;

    State GetState() const { return m_state; }
    const DirectX::SimpleMath::Vector3& Direction() const { return m_direction; }
    const DirectX::SimpleMath::Vector3& PreviewStart() const { return m_origin; }
    DirectX::SimpleMath::Vector3 PreviewEnd() const { return m_origin + m_direction * m_distance; }
    bool HasHit() const { return m_hit; }
    void MarkHit() { m_hit = true; }

    float Progress() const
    {
        if (m_state == State::Windup) return std::clamp(m_elapsed / m_windup, 0.0f, 1.0f);
        if (m_state == State::Charging) return std::clamp(m_traveled / m_distance, 0.0f, 1.0f);
        if (m_state == State::Recovery) return std::clamp(m_elapsed / m_recovery, 0.0f, 1.0f);
        return 0;
    }

    Movement Update(float deltaSeconds, DirectX::SimpleMath::Vector3& position,
                    const DirectX::SimpleMath::Vector3& player, float triggerRange)
    {
        using DirectX::SimpleMath::Vector3;
        Movement movement;
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0) return movement;
        float remaining = deltaSeconds;
        while (remaining > 0.000001f)
        {
            if (m_state == State::Chase)
            {
                Vector3 toPlayer = player - position;
                toPlayer.y = 0;
                const float distance = toPlayer.Length();
                if (distance > 0.0001f) m_direction = toPlayer / distance;
                const float range = (std::max)(triggerRange, 30.0f);
                if (distance <= range + 0.001f && m_cooldown <= 0.000001f)
                {
                    // この瞬間の方向・距離を固定する。予測線を出してからPlayerを追尾しない。
                    m_origin = position;
                    m_distance = (std::max)(chargeDistance, 1.0f);
                    m_speed = (std::max)(chargeSpeed, 1.0f);
                    m_windup = (std::max)(windupSeconds, 0.05f);
                    m_recovery = (std::max)(recoverySeconds, 0.05f);
                    m_elapsed = m_traveled = 0;
                    m_hit = false;
                    m_state = State::Windup;
                    continue;
                }
                const float speed = (std::max)(walkSpeed, 1.0f);
                const float untilRange = (std::max)(distance - range, 0.0f) / speed;
                const float step = (std::min)(remaining, (std::max)(untilRange, m_cooldown));
                // 近づきすぎても体の中心まで重ならず、突進開始を待つ。
                position += m_direction * (std::min)(speed * step, (std::max)(distance - 28.0f, 0.0f));
                m_cooldown = (std::max)(m_cooldown - step, 0.0f);
                remaining -= step;
            }
            else if (m_state == State::Charging)
            {
                const float step = (std::min)(remaining, (m_distance - m_traveled) / m_speed);
                if (!movement.charged) movement.start = position;
                movement.charged = true;
                m_traveled = (std::min)(m_distance, m_traveled + step * m_speed);
                position = m_origin + m_direction * m_traveled;
                movement.end = position;
                remaining -= step;
                if (m_traveled >= m_distance - 0.0001f)
                {
                    m_state = State::Recovery;
                    m_elapsed = 0;
                }
            }
            else
            {
                const float duration = m_state == State::Windup ? m_windup : m_recovery;
                const float step = (std::min)(remaining, duration - m_elapsed);
                m_elapsed += step;
                remaining -= step;
                if (m_elapsed >= duration - 0.000001f)
                {
                    if (m_state == State::Windup) m_state = State::Charging;
                    else
                    {
                        m_state = State::Chase;
                        m_cooldown = (std::max)(cooldownSeconds, 0.0f);
                    }
                    m_elapsed = 0;
                }
            }
        }
        return movement;
    }

private:
    State m_state = State::Chase;
    DirectX::SimpleMath::Vector3 m_direction{0, 0, 1}, m_origin{};
    float m_elapsed = 0, m_traveled = 0, m_cooldown = 0;
    float m_distance = 240, m_speed = 210, m_windup = 1, m_recovery = 0.8f;
    bool m_hit = false;
};
