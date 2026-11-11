/// @file Bezier.cpp
/// @brief 距離に応じた所要時間で進むミサイルのベジェ軌道。

#include "Bezier.h"

#include <algorithm>

void Bezier::Create(Object& Player, Object& Enemy, float angleOffsetDebug) {
    Create(Player.GetPosition(), Enemy.GetPosition(), angleOffsetDebug);
}

void Bezier::Create(const DirectX::SimpleMath::Vector3& StartPos, const DirectX::SimpleMath::Vector3& EndPos,
                    float angleOffsetDebug) {
    m_positions.clear();

    DirectX::SimpleMath::Vector3 dir = EndPos - StartPos;
    dir.y = 0.0f; // 水平方向のみにする
    float dist = dir.Length();
    if (dist > 0.0001f) { // 長さが十分にある場合のみ正規化
        dir.Normalize();
    } else {
        dir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f); // デフォルトの方向
    }

    //指定された角度からY軸周りの回転を計算
    float rad = DirectX::XMConvertToRadians(angleOffsetDebug);
    DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(rad);
    DirectX::SimpleMath::Vector3 offsetDir = DirectX::SimpleMath::Vector3::TransformNormal(dir, rot);

    //制御点の設定
    DirectX::SimpleMath::Vector3 controlPoint = StartPos + offsetDir * (dist * 0.5f);
    // 近い敵には低い軌道で飛ばし、固定の高さまで遠回りするのを防ぐ。
    // 空中の敵も始点と終点の中間の高さを基準にする。
    const float distance = (EndPos - StartPos).Length();
    controlPoint.y = (StartPos.y + EndPos.y) * 0.5f + (std::min)(100.0f, distance * 0.35f);

    m_positions.push_back(StartPos);
    m_positions.push_back(controlPoint);
    m_positions.push_back(EndPos);

    // 山なり・左右に広がる軌道も含め、曲線を64分割して飛行距離を見積もる。
    // 一律1秒で進めるのではなく、「経路の長さ / 基準速度」を所要時間にする。
    constexpr int segments = 64;
    float pathLength = 0.0f;
    DirectX::SimpleMath::Vector3 previous = StartPos;
    for (int i = 1; i <= segments; ++i) {
        const DirectX::SimpleMath::Vector3 position = GetCulvePosition(static_cast<float>(i) / segments);
        pathLength += (position - previous).Length();
        previous = position;
    }

    // 同じ位置で発射されても0除算しない。追尾中は所要時間を再設定せず、進行を保つ。
    m_flightDurationSeconds = (std::max)(0.001f, pathLength / FlightSpeed);

    m_time = 0.0f;
    m_active = true;
}

void Bezier::Update(float tick) {
    if (!m_active || tick <= 0.0f) {
        return;
    }

    // tickは実測秒数。近距離ほど早く、遠距離ほど時間をかけて終点へ進む。
    m_time += tick / m_flightDurationSeconds;

    if (m_time >= 1.0f) {
        m_time = 1.0f;
        m_active = false;
    }
}

DirectX::XMFLOAT3 Bezier::GetCulvePosition(float t) {
    //POWは使わず乗算で軽くする

    // ベジェ曲線計算
    DirectX::XMFLOAT3 outpos;

    // 2次ベジェ曲線の計算 (制御点3つ)
    float mt = 1.0f - t;
    float mt2 = mt * mt; // (1-t)^2
    float t2 = t * t; // t^2

    // P(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
    outpos.x = mt2 * m_positions[0].x + 2.0f * mt * t * m_positions[1].x + t2 * m_positions[2].x;
    outpos.y = mt2 * m_positions[0].y + 2.0f * mt * t * m_positions[1].y + t2 * m_positions[2].y;
    outpos.z = mt2 * m_positions[0].z + 2.0f * mt * t * m_positions[1].z + t2 * m_positions[2].z;

    return outpos;
}
