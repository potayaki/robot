#pragma once

#include <DirectXMath.h>
#include<vector>
#include"Object.h"
class Bezier
{
public:
    Bezier() = default;

    void Create(Object& Player, Object& Enemy,float angleOffsetDebug);

    void Update(float tick);

    DirectX::XMFLOAT3 GetCulvePosition(float t);

    float GetTime() { return m_Time; }

    bool IsActive() { return mActive; }

    void UpdateTargetPosition(DirectX::SimpleMath::Vector3 targetpos) {
        if (!m_Positions.empty()) {
            m_Positions.back() = targetpos;
        }
    }

private:

    std::vector<DirectX::SimpleMath::Vector3>m_Positions;

    float m_Time = 0.0f;
    float mSpeed = 1.0f;
    bool mActive = false;//表示
};
