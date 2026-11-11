/// @file Bezier.h
/// @brief 距離に応じた所要時間で進むミサイルのベジェ軌道。

#pragma once
/*
ベジエ曲線CMissileで使われている
*/
#include <DirectXMath.h>
#include <vector>
#include "Object.h"

class Bezier {
public:
    Bezier() = default;

    void Create(Object& Player, Object& Enemy, float angleOffsetDebug);

    // 座標からの生成にも対応し、飛行時間を描画なしで検証できるようにする。
    void Create(const DirectX::SimpleMath::Vector3& start, const DirectX::SimpleMath::Vector3& end,
                float angleOffsetDebug);

    float GetFlightDurationSeconds() const {
        return m_flightDurationSeconds;
    }

    // 発射時の経路全体に対する基準速度（ゲーム内の距離/秒）。
    /// ミサイルが軌道を進む基準速度（ワールド単位/秒）。
    static constexpr float FlightSpeed = 600.0f;

    /// 経過時間と入力に合わせて状態を更新する。
    void Update(float tick);

    DirectX::XMFLOAT3 GetCulvePosition(float t);

    float GetTime() {
        return m_time;
    }

    bool IsActive() {
        return m_active;
    }

    void UpdateTargetPosition(DirectX::SimpleMath::Vector3 targetpos) {
        if (!m_positions.empty()) {
            m_positions.back() = targetpos;
        }
    }

private:
    /// ベジェ軌道の始点・制御点・終点（ワールド座標）。
    std::vector<DirectX::SimpleMath::Vector3> m_positions;

    /// ベジェ軌道の進行率。0で発射、1で到着を表す。
    float m_time = 0.0f;
    /// 軌道の長さから求めた発射から到着までの所要秒数。
    float m_flightDurationSeconds = 1.0f;
    /// 表示
    bool m_active = false;
};
