#pragma once
/*
ベジエ曲線CMissileで使われている
*/
#include <DirectXMath.h>
#include <vector>
#include "Object.h"
class Bezier
{
public:
    Bezier() = default;

    void Create(Object &Player, Object &Enemy, float angleOffsetDebug);

    // 座標からの生成にも対応し、飛行時間を描画なしで検証できるようにする。
    void Create(const DirectX::SimpleMath::Vector3 &start, const DirectX::SimpleMath::Vector3 &end,
                float angleOffsetDebug);

    float GetFlightDurationSeconds() const
    {
        return m_flightDurationSeconds;
    }

    // 発射時の経路全体に対する基準速度（ゲーム内の距離/秒）。
    static constexpr float FlightSpeed = 600.0f;

    void Update(float tick);

    DirectX::XMFLOAT3 GetCulvePosition(float t);

    float GetTime()
    {
        return m_Time;
    }

    bool IsActive()
    {
        return mActive;
    }

    void UpdateTargetPosition(DirectX::SimpleMath::Vector3 targetpos)
    {
        if (!m_Positions.empty())
        {
            m_Positions.back() = targetpos;
        }
    }

private:
    std::vector<DirectX::SimpleMath::Vector3> m_Positions;

    float m_Time = 0.0f;
    float m_flightDurationSeconds = 1.0f;
    bool mActive = false; //表示
};
