#pragma once

#include <DirectXMath.h>
#include<vector>
#include"Object.h"
class Bezier
{
public:
    Bezier() = default;

    void Create(Object& Player, Object& Enemy);

    void Update(float tick);

    DirectX::XMFLOAT3 GetCulvePosition(float t);

private:

    std::vector<DirectX::SimpleMath::Vector3>m_Positions;

    float m_Time = 0.0f;
    float mSpeed = 1.0f;
    bool mActive = false;
};
