#pragma once
#include <algorithm>
#include <cmath>

namespace BulletHoming {
    // 通常弾の速度・寿命は変えず、Lv1～5で1秒あたり24～120度まで曲げる。
    constexpr int MaxRank = 5;
    constexpr float SearchRadius = 600.0f;
    constexpr float DegreesPerSecond(int rank) {
        return 24.0f * (rank < 0 ? 0 : (rank > MaxRank ? MaxRank : rank));
    }

    // 計算を描画から独立させる。ゲームではSimpleMath::Vector3を使用する。
    template<class Vector>
    class Search {
        Vector m_velocity;
        Vector m_offset{};
        float m_distanceSquared = SearchRadius * SearchRadius;
        bool m_found = false;
    public:
        explicit Search(const Vector& velocity) : m_velocity(velocity) {}
        void Consider(const Vector& offset) {
            const float distanceSquared = offset.LengthSquared();
            // 通過済みの敵は追わず、その先にいる敵を新たな誘導先にする。
            if (distanceSquared <= 0.000001f || distanceSquared > m_distanceSquared ||
                m_velocity.Dot(offset) <= 0.0f) return;
            m_found = true;
            m_distanceSquared = distanceSquared;
            m_offset = offset;
        }
        bool Found() const { return m_found; }
        const Vector& Offset() const { return m_offset; }
    };

    template<class Vector>
    Vector Turn(const Vector& velocity, const Vector& offset, int rank) {
        const float speed = velocity.Length();
        const float distance = offset.Length();
        if (rank <= 0 || speed < 0.000001f || distance < 0.000001f ||
            velocity.Dot(offset) <= 0.0f) return velocity;
        const Vector forward = velocity / speed;
        const Vector target = offset / distance;
        const float dot = std::clamp(forward.Dot(target), 0.0f, 1.0f);
        constexpr float Pi = 3.14159265358979323846f;
        // 既存の移動・寿命と同じ60更新/秒。一定角度以上は曲げず、必中にしない。
        const float step = DegreesPerSecond(rank) * Pi / 180.0f / 60.0f;
        if (std::acos(dot) <= step) return target * speed;
        Vector tangent = target - forward * dot;
        if (tangent.LengthSquared() < 0.000001f) return velocity;
        tangent.Normalize();
        Vector result = forward * std::cos(step) + tangent * std::sin(step);
        result.Normalize(); // 誘導を繰り返しても弾速を変えない。
        return result * speed;
    }
}
