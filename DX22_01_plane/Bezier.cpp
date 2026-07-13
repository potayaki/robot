#include "Bezier.h"

void Bezier::Create(Object& Player, Object& Enemy) {
    m_Positions.clear();

    auto StartPos = Player.GetPosition();
    auto EndPos = Enemy.GetPosition();
    auto ControlPos = DirectX::XMFLOAT3(
        (StartPos.x + EndPos.x) / 2.0f,
        10.0f,
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

    if (m_Time <= 1.0f) {

        m_Time = 1.0f;
        mActive = false;

    }

    m_Time += tick;


}

DirectX::XMFLOAT3 Bezier::GetCulvePosition(float t) {

    // ベジェ曲線計算
    DirectX::XMFLOAT3 outpos;

    // 課題範囲--------------------------------------------------------------------------

    outpos.x = pow(1 - t, 3) * m_Positions[0].x + 3 * pow(1 - t, 2) * t * m_Positions[1].x + 3 * (1 - t) * pow(t, 2) * m_Positions[2].x + pow(t, 3) * m_Positions[3].x;
    outpos.y = pow(1 - t, 3) * m_Positions[0].y + 3 * pow(1 - t, 2) * t * m_Positions[1].y + 3 * (1 - t) * pow(t, 2) * m_Positions[2].y + pow(t, 3) * m_Positions[3].y;
    outpos.z = pow(1 - t, 3) * m_Positions[0].z + 3 * pow(1 - t, 2) * t * m_Positions[1].z + 3 * (1 - t) * pow(t, 2) * m_Positions[2].z + pow(t, 3) * m_Positions[3].z;

    return outpos;
}
