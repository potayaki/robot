#include "Bezier.h"

void Bezier::Create(Object& Player, Object& Enemy) {
    m_Positions.clear();

    auto StartPos = Player.GetPosition();
    auto EndPos = Enemy.GetPosition();
    auto ControlPos = DirectX::XMFLOAT3(
        (StartPos.x + EndPos.x) / 2.0f,
        100.0f,
        (StartPos.z + EndPos.z) / 2.0f
    );

    m_Positions.push_back(StartPos);
    m_Positions.push_back(ControlPos);
    m_Positions.push_back(EndPos);

    m_Time = 0.0f;
    mActive = true;

}

void Bezier::Update(float tick) {
    if (!mActive) return;

    m_Time += tick * mSpeed;

    if (m_Time >= 1.0f) {

        m_Time = 1.0f;
        mActive = false;

    }

}

DirectX::XMFLOAT3 Bezier::GetCulvePosition(float t) {
    //POWは使わず乗算で軽くする

    // ベジェ曲線計算
    DirectX::XMFLOAT3 outpos;

    // 2次ベジェ曲線の計算 (制御点3つ)
    float mt = 1.0f - t;
    float mt2 = mt * mt; // (1-t)^2
    float t2 = t * t;    // t^2

    // P(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
    outpos.x = mt2 * m_Positions[0].x + 2.0f * mt * t * m_Positions[1].x + t2 * m_Positions[2].x;
    outpos.y = mt2 * m_Positions[0].y + 2.0f * mt * t * m_Positions[1].y + t2 * m_Positions[2].y;
    outpos.z = mt2 * m_Positions[0].z + 2.0f * mt * t * m_Positions[1].z + t2 * m_Positions[2].z;


    return outpos;
}
