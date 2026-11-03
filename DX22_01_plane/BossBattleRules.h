#pragma once
#include <SimpleMath.h>
#include <algorithm>

// 描画やオブジェクトの寿命に依存しない、ボス戦の進行・境界判定。
class BossBattleProgress
{
    int m_checkpointKills = 0;
    bool m_active = false;

public:
    int killsPerBoss = 30;

    bool IsActive() const { return m_active; }

    int RemainingKills(int kills) const
    {
        const int earned = (std::max)(0, kills - m_checkpointKills);
        return (std::max)(0, (std::max)(1, killsPerBoss) - earned);
    }

    bool ShouldStart(int kills) const
    {
        return !m_active && RemainingKills(kills) == 0;
    }

    void Begin() { m_active = true; }

    void Finish(int kills)
    {
        // ボス撃破分は次の条件に数えず、通常戦に戻った時点から数え直す。
        m_checkpointKills = kills;
        m_active = false;
    }
};

namespace BossArena
{
    inline DirectX::SimpleMath::Vector3 ClampPosition(
        const DirectX::SimpleMath::Vector3& position,
        const DirectX::SimpleMath::Vector3& center, float radius, float bodyRadius)
    {
        using DirectX::SimpleMath::Vector3;
        Vector3 offset = position - center;
        offset.y = 0.0f;
        const float limit = (std::max)(0.0f, radius - bodyRadius);
        const float distance = offset.Length();
        if (distance <= limit || distance <= 0.00001f)
        {
            return position;
        }

        // XZ平面だけを制限する。ジャンプの高さを変えず、壁沿いには移動できる。
        const Vector3 boundary = center + offset * (limit / distance);
        return Vector3(boundary.x, position.y, boundary.z);
    }
}
