/// @file Collision.h
/// @brief 球、線分、平面、三角形などの当たり判定と距離計算。

//================================
// Collision.h　v1.0
//================================
#pragma once

#include <simplemath.h>
#include <cmath>

namespace Collision {
    // ライン（無限の長さの線）
    struct Line {
        /// 通過地点
        DirectX::SimpleMath::Vector3 point;
        /// 線の方向ベクトル
        DirectX::SimpleMath::Vector3 vec;
    };

    // プレーン(無限の広さの平面)
    struct Plane {
        /// 平面上の1点
        DirectX::SimpleMath::Vector3 point;
        /// 平面の法線ベクトル
        DirectX::SimpleMath::Vector3 normal;
        //float d;         // 平面の方程式: ax + by + cz + d = 0 の d
    };

    // セグメント（有限の長さの線分）
    struct Segment {
        /// 始点
        DirectX::SimpleMath::Vector3 start;
        /// 終点
        DirectX::SimpleMath::Vector3 end;
    };

    // 三角形ポリゴン（有限の広さの平面）
    struct Polygon {
        /// 頂点0
        const DirectX::SimpleMath::Vector3 p0;
        /// 頂点1
        const DirectX::SimpleMath::Vector3 p1;
        /// 頂点2
        const DirectX::SimpleMath::Vector3 p2;
    };

    // 球体
    struct Sphere {
        /// 中心
        DirectX::SimpleMath::Vector3 center;
        /// 半径
        float radius;
    };

    /// 移動球が相手の球に初めて触れる位置を、線分の始点0～終点1で返す。
    inline bool SegmentSphereHitFraction(const Segment& segment, const Sphere& sphere, float movingRadius,
                                         float& hitFraction) {
        const auto fromCenter = segment.start - sphere.center;
        const float radius = sphere.radius + movingRadius;
        const float c = fromCenter.LengthSquared() - radius * radius;
        if (c <= 0.0f) {
            hitFraction = 0.0f; // 停止中でも始点が球内ならすでに接触している。
            return true;
        }

        const auto step = segment.end - segment.start;
        const float b = fromCenter.Dot(step);
        if (b >= 0.0f) {
            return false; // 球外から止まっているか、遠ざかる向きに動いた。
        }
        const float discriminant = b * b - step.LengthSquared() * c;
        if (discriminant < 0.0f) {
            return false;
        }
        // 手前の交点。始点のすぐ近くでも、小さい数同士の差で精度を失わない形を使う。
        hitFraction = c / (-b + std::sqrt(discriminant));
        return hitFraction <= 1.0f;
    }

    // BOX定義
    struct AABB {
        /// 各軸の最小座標で表した境界の下限。
        DirectX::SimpleMath::Vector3 min;
        /// 各軸の最大座標で表した境界の上限。
        DirectX::SimpleMath::Vector3 max;
    };

    //当たり判定
    bool CheckHit(const Line& line, const Plane& plane); //線(無限の長さ)と平面(無限の大きさ)
    bool CheckHit(const Segment& segment, const Plane& plane); //線分と平面(無限の大きさ)
    bool CheckHit(const Line& line, const Polygon& polygon); //線(無限の長さ)とポリゴン
    bool CheckHit(const Line& line, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
    bool CheckHit(const Segment& segment, const Polygon& polygon); //線分とポリゴン
    bool CheckHit(const Segment& segment, const Polygon& polygon,
                  DirectX::SimpleMath::Vector3& contact); //同上
    bool CheckHit(const Sphere& sphere, const Plane& plane); //球体と平面(無限の大きさ)
    bool CheckHit(const Sphere& sphere, const Polygon& polygon); //球体とポリゴン
    bool CheckHit(const Sphere& sphere, const Polygon& polygon, DirectX::SimpleMath::Vector3& contact); //同上
    bool CheckHit(Sphere sphere1, Sphere sphere2); //球体と球体
    bool CheckHit(Sphere sphere1, Sphere sphere2, DirectX::SimpleMath::Vector3& contact); //同上
    bool CheckHit(AABB p1, AABB p2); // AABBとAABB

    //内積・外積
    float Dot(const DirectX::SimpleMath::Vector3& v1, const DirectX::SimpleMath::Vector3& v2);
    DirectX::SimpleMath::Vector3 Cross(const DirectX::SimpleMath::Vector3& v1,
                                       const DirectX::SimpleMath::Vector3& v2);

    DirectX::SimpleMath::Vector3 ClosestPointOnSegment(const DirectX::SimpleMath::Vector3& point,
                                                       const Segment& segment);
    float DistanceSquaredPointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment);
    float DistanceSquaredPointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment,
                                        DirectX::SimpleMath::Vector3& contact);
    float DistancePointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment);
    float DistancePointToSegment(const DirectX::SimpleMath::Vector3& point, const Segment& segment,
                                 DirectX::SimpleMath::Vector3& contact);
    float DistancePointToPlane(const DirectX::SimpleMath::Vector3& point, const Plane& plane);
    DirectX::SimpleMath::Vector3 ProjectPointToPlane(const DirectX::SimpleMath::Vector3& point,
                                                     const Plane& plane);
    bool PointInTriangle(const DirectX::SimpleMath::Vector3& point, const Polygon& polygon);
    DirectX::SimpleMath::Vector3 ClosestPointOnTriangle(const DirectX::SimpleMath::Vector3& point,
                                                        const Polygon& polygon);
    DirectX::SimpleMath::Vector3 GetNormal(const Polygon& polygon);

    //球体を当たった後の地点に動かす
    DirectX::SimpleMath::Vector3 moveSphere(const Segment& capsule, const float& radius,
                                            const Polygon& polygon,
                                            const DirectX::SimpleMath::Vector3& contact, float& distance);
    DirectX::SimpleMath::Vector3 moveSphere(const Sphere& sphere, const Polygon& polygon,
                                            const DirectX::SimpleMath::Vector3& contact);

    AABB SetAABB(DirectX::SimpleMath::Vector3 centerposition, float width, float height, float depth);
} // namespace Collision
