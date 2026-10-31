#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <cmath>
#include <vector>

// 目の寸法・向き・球の形状をまとめる。描画とレーザーが同じ基準を使う。
namespace EyeTurretShape
{
    using DirectX::SimpleMath::Matrix;
    using DirectX::SimpleMath::Quaternion;
    using DirectX::SimpleMath::Vector3;

    constexpr float Radius = 11.0f;
    constexpr float Height = 18.0f; // 設置位置から浮かせる高さ。
    constexpr float TurnSpeedRadiansPerSecond = 6.2831853f; // 360度/秒。背後には約0.5秒で振り向く。

    inline Vector3 CompanionAnchor(const Vector3& playerPosition, float playerYaw)
    {
        // プレイヤーの右後方・少し上。瞳の中心はさらにHeightだけ上に描画する。
        const Vector3 forward(std::sin(playerYaw), 0, std::cos(playerYaw));
        const Vector3 right(std::cos(playerYaw), 0, -std::sin(playerYaw));
        return playerPosition - forward * 35.0f + right * 28.0f + Vector3(0, 8, 0);
    }

    inline Vector3 FollowPosition(const Vector3& current, const Vector3& target, float seconds)
    {
        // 秒数から補間率を求め、FPSに依存せず少し遅れて滑らかについてくる。
        const float amount = 1.0f - std::exp(-7.0f * (std::max)(0.0f, seconds));
        return Vector3::Lerp(current, target, amount);
    }

    inline Vector3 DirectionTo(const Vector3& center, const Vector3& target, const Vector3& fallback)
    {
        Vector3 direction = target - center;
        if (direction.LengthSquared() < 0.000001f)
        {
            return fallback; // 敵と同じ位置でも0ベクトルを正規化しない。
        }
        direction.Normalize();
        return direction;
    }

    inline Vector3 LookDirection(const Quaternion& rotation)
    {
        // ローカル+Zが瞳の正面。描画と発射位置は同じ姿勢から求める。
        return Vector3::Transform(Vector3(0, 0, 1), rotation);
    }

    // 現在の姿勢から標的へSlerpで回す。向き終わった場合だけtrueを返す。
    inline bool TurnTowards(Quaternion& rotation, const Vector3& targetDirection, float deltaSeconds)
    {
        const Vector3 forward = LookDirection(rotation);
        const Vector3 desired = DirectionTo(Vector3(0, 0, 0), targetDirection, forward);
        Vector3 axis = forward.Cross(desired);
        const float axisLength = axis.Length();
        const float dot = std::clamp(forward.Dot(desired), -1.0f, 1.0f);
        const float angle = std::atan2(axisLength, dot);

        if (angle < 0.000001f)
        {
            return true;
        }
        if (deltaSeconds <= 0.0f)
        {
            return false; // 一時停止中には姿勢を変えない。
        }

        if (axisLength < 0.000001f)
        {
            // 真後ろは回転軸が決まらないため、現在の目の上方向を軸に振り向く。
            axis = Vector3::Transform(Vector3(0, 1, 0), rotation);
        }
        else
        {
            axis /= axisLength;
        }

        // 現在の姿勢へ必要な回転だけ加える。真上を通過しても上下の基準を切り替えず、
        // 虹彩の模様が突然ねじれるのを防ぐ。毎回の補間率は実測秒数と残り角度で決める。
        const Quaternion turn = Quaternion::CreateFromAxisAngle(axis, angle);
        const Quaternion targetRotation = rotation * turn;
        const float maxAngle = TurnSpeedRadiansPerSecond * deltaSeconds;
        const float amount = (std::min)(1.0f, maxAngle / angle);
        rotation = Quaternion::Slerp(rotation, targetRotation, amount);
        rotation.Normalize();
        return maxAngle >= angle;
    }

    inline Matrix WorldMatrix(const Vector3& center, const Quaternion& rotation)
    {
        return Matrix::CreateScale(Radius) * Matrix::CreateFromQuaternion(rotation) *
               Matrix::CreateTranslation(center);
    }

    inline Vector3 BeamStart(const Vector3& center, const Vector3& target, const Vector3& direction)
    {
        // 瞳の表面の少し外側から発射する。敵が目に重なっても逆向きの線にしない。
        const float distance = (target - center).Length();
        const float muzzleOffset = (std::min)(Radius + 0.2f, distance);
        return center + direction * muzzleOffset;
    }

    struct SphereMesh
    {
        std::vector<Vector3> positions;
        std::vector<unsigned int> indices;
    };

    inline SphereMesh BuildSphere()
    {
        constexpr unsigned int rings = 32;
        constexpr unsigned int segments = 64;
        constexpr float pi = 3.14159265358979323846f;
        SphereMesh mesh;
        mesh.positions.reserve((rings + 1) * (segments + 1));
        mesh.indices.reserve((rings - 1) * segments * 6);

        // 単位球を作り、描画時にRadius倍する。法線は位置と同じ向きなので滑らかになる。
        for (unsigned int ring = 0; ring <= rings; ++ring)
        {
            const float angle = pi * ring / rings;
            for (unsigned int segment = 0; segment <= segments; ++segment)
            {
                const float around = 2.0f * pi * segment / segments;
                mesh.positions.emplace_back(std::sin(angle) * std::cos(around),
                                            std::sin(angle) * std::sin(around), std::cos(angle));
            }
        }

        for (unsigned int ring = 0; ring < rings; ++ring)
        {
            for (unsigned int segment = 0; segment < segments; ++segment)
            {
                const unsigned int a = ring * (segments + 1) + segment;
                const unsigned int b = a + segments + 1;
                // 球の両端では面積0の三角形を作らない。表側が外を向く順に並べる。
                if (ring < rings - 1)
                {
                    mesh.indices.insert(mesh.indices.end(), {a, b, b + 1});
                }
                if (ring > 0)
                {
                    mesh.indices.insert(mesh.indices.end(), {a, b + 1, a + 1});
                }
            }
        }
        return mesh;
    }
}
