#pragma once
#include <SimpleMath.h>
#include <cmath>
#include "BulletHoming.h"

namespace AimCursor
{
    // 表示サイズと対象判定で同じ値を使い、カーソルの見た目と誘導範囲を揃える。
    constexpr float MissileDiameter = 256.0f;
    // crosshair1310.pngは128px画像の中心から約60pxの位置に円の外周がある。
    constexpr float MissileRadius = MissileDiameter * (60.0f / 128.0f);
    constexpr float FieldOfView = DirectX::XM_PI / 4.0f;

    // 現在のカメラとカーソルから作る、ミサイル・誘導弾共通の探索範囲。
    // 描画済みの行列には依存しないため、HUDが2Dカメラを使った後でも正しく判定できる。
    class Region
    {
        using Vector3 = DirectX::SimpleMath::Vector3;
        using Vector2 = DirectX::SimpleMath::Vector2;
        Vector3 m_origin, m_forward, m_right, m_up, m_cursorRay;
        Vector2 m_cursor;
        float m_focalLength = 0.0f;
        bool m_valid = false;

    public:
        Region(const Vector3& origin, const Vector3& target, const Vector2& cursor, float screenHeight)
            : m_origin(origin), m_forward(target - origin), m_cursor(cursor)
        {
            if (screenHeight <= 0 || m_forward.LengthSquared() < 0.000001f) return;
            m_forward.Normalize();
            m_right = Vector3(0, 1, 0).Cross(m_forward);
            if (m_right.LengthSquared() < 0.000001f) return;
            m_right.Normalize();
            m_up = m_forward.Cross(m_right);
            m_focalLength = screenHeight * 0.5f / std::tan(FieldOfView * 0.5f);
            m_cursorRay = m_forward * m_focalLength + m_right * cursor.x + m_up * cursor.y;
            m_cursorRay.Normalize();
            m_valid = true;
        }

        // 画面上のカーソル中心からの距離の二乗。弾から敵へのワールド距離とは区別する。
        bool TryGetCursorDistanceSquared(const Vector3& enemyCenter, float& distanceSquared) const
        {
            if (!m_valid) return false;
            const Vector3 toEnemy = enemyCenter - m_origin;
            const float depth = toEnemy.Dot(m_forward);
            if (depth <= 0.0001f) return false; // カメラの背後は候補にしない。

            // 敵の中心を画面中央原点・Y上向きのUI座標へ投影し、表示中の円に入るか調べる。
            // 角度だけの判定と違い、画面端を狙っても円の外側へ誘導されない。
            const float x = toEnemy.Dot(m_right) * m_focalLength / depth - m_cursor.x;
            const float y = toEnemy.Dot(m_up) * m_focalLength / depth - m_cursor.y;
            distanceSquared = x * x + y * y;
            return std::isfinite(distanceSquared);
        }

        bool TryGetBulletScore(const Vector3& enemyCenter, int rank, float& distanceSquared) const
        {
            const float radius = BulletHoming::CursorRadius(rank);
            // 弾用カーソルは「敵の中心が枠の内側」に入ったときだけ候補にする。
            return radius > 0.0f && TryGetCursorDistanceSquared(enemyCenter, distanceSquared) &&
                   distanceSquared <= radius * radius;
        }

        bool Contains(const Vector3& enemyCenter, float enemyRadius) const
        {
            float distanceSquared = 0.0f;
            if (!TryGetCursorDistanceSquared(enemyCenter, distanceSquared)) return false;
            if (distanceSquared <= MissileRadius * MissileRadius) return true;

            // 大型ボスの中心が円外でも、照準の中心が体を直接捉えていれば対象にする。
            // これは従来のミサイルにもあった補助判定。遠くの別方向の敵は対象にならない。
            const Vector3 toEnemy = enemyCenter - m_origin;
            const float alongRay = toEnemy.Dot(m_cursorRay);
            if (enemyRadius <= 0.0f || alongRay <= 0.0f) return false;
            const Vector3 perpendicular = toEnemy - m_cursorRay * alongRay;
            return perpendicular.LengthSquared() <= enemyRadius * enemyRadius;
        }
    };
}
