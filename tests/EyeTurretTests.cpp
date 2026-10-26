#include "../DX22_01_plane/EyeTurretShape.h"
#include <cassert>
#include <iostream>

using namespace DirectX::SimpleMath;

namespace
{
    float RotationAngle(const Quaternion& from, const Quaternion& to)
    {
        const float dot = std::clamp(std::abs(from.Dot(to)), 0.0f, 1.0f);
        return 2.0f * std::acos(dot);
    }

    Quaternion SimulateTurn(const Vector3& target, float seconds, float frameSeconds)
    {
        Quaternion rotation;
        while (seconds > 0.000001f)
        {
            const float step = (std::min)(seconds, frameSeconds);
            EyeTurretShape::TurnTowards(rotation, target, step);
            seconds -= step;
        }
        return rotation;
    }

    void TestSmoothRotation()
    {
        const Vector3 behind(0, 0, -1);
        Quaternion rotation;
        const Quaternion before = rotation;

        // 真後ろの敵へも1フレームで飛ばず、設定した回転速度の範囲内で回る。
        assert(!EyeTurretShape::TurnTowards(rotation, behind, 1.0f / 60.0f));
        const float stepAngle = RotationAngle(before, rotation);
        assert(stepAngle > 0.05f);
        assert(stepAngle <= EyeTurretShape::TurnSpeedRadiansPerSecond / 60.0f + 0.0001f);

        const Quaternion paused = rotation;
        assert(!EyeTurretShape::TurnTowards(rotation, behind, 0.0f));
        assert(rotation == paused);
        assert(!EyeTurretShape::TurnTowards(rotation, behind, -0.1f));
        assert(rotation == paused);

        // 30/60/144fpsで、同じ0.2秒後の姿勢になる。真後ろと斜め上の両方を確認する。
        for (const Vector3 target : {behind, Vector3(-1, 2, -3)})
        {
            const Quaternion at30 = SimulateTurn(target, 0.2f, 1.0f / 30.0f);
            const Quaternion at60 = SimulateTurn(target, 0.2f, 1.0f / 60.0f);
            const Quaternion at144 = SimulateTurn(target, 0.2f, 1.0f / 144.0f);
            assert(std::abs(at30.Dot(at60)) > 0.99999f);
            assert(std::abs(at30.Dot(at144)) > 0.99999f);

            const Quaternion reached = SimulateTurn(target, 0.6f, 1.0f / 60.0f);
            Vector3 expected = target;
            expected.Normalize();
            assert((EyeTurretShape::LookDirection(reached) - expected).Length() < 0.00001f);
        }

        // 上下左右の回転が混ざった姿勢からでも、正しい敵の方向へ向き終える。
        rotation = Quaternion::CreateFromYawPitchRoll(0.8f, -0.7f, 0.4f);
        for (const Vector3 target : {Vector3(1, 0, 0), Vector3(0, -1, 0), Vector3(-1, 2, -3)})
        {
            assert(EyeTurretShape::TurnTowards(rotation, target, 1.0f));
            Vector3 expected = target;
            expected.Normalize();
            assert((EyeTurretShape::LookDirection(rotation) - expected).Length() < 0.00001f);
            assert(std::abs(rotation.Length() - 1.0f) < 0.00001f);
        }

        // 真上を通り越す敵を追っても、虹彩ごと180度ねじれるような姿勢の飛びを起こさない。
        rotation = Quaternion{};
        for (int frame = 0; frame <= 120; ++frame)
        {
            const float angle = frame * 3.14159265f / 120.0f;
            const Vector3 target(0, std::sin(angle), std::cos(angle));
            const Quaternion previous = rotation;
            assert(EyeTurretShape::TurnTowards(rotation, target, 1.0f / 60.0f));
            assert((EyeTurretShape::LookDirection(rotation) - target).Length() < 0.00001f);
            assert(RotationAngle(previous, rotation) < 0.03f);
        }
    }
}

int main()
{
    TestSmoothRotation();
    const Vector3 center(15, 23, -45);
    // 地上・空中・背後・真上・真下の敵すべてで、瞳の正面とビームを一致させる。
    for (const Vector3 offset :
         {Vector3(100, 0, 0), Vector3(-100, 0, 0), Vector3(0, 0, 100), Vector3(0, 0, -100),
          Vector3(20, 80, 45), Vector3(0, 100, 0), Vector3(0, -100, 0)})
    {
        const Vector3 target = center + offset;
        const Vector3 direction = EyeTurretShape::DirectionTo(center, target, Vector3(0, 0, 1));
        Quaternion rotation;
        assert(EyeTurretShape::TurnTowards(rotation, direction, 1.0f));
        const Matrix world = EyeTurretShape::WorldMatrix(center, rotation);
        const Vector3 pupil = Vector3::Transform(Vector3(0, 0, 1), world);
        const Vector3 muzzle = EyeTurretShape::BeamStart(center, target, direction);
        assert((pupil - center - direction * EyeTurretShape::Radius).Length() < 0.0001f);
        assert((muzzle - center).Cross(target - center).Length() < 0.002f);
        assert((muzzle - pupil).Dot(direction) > 0.0f);  // 発射位置は瞳の表面より外側。
        assert((target - muzzle).Dot(direction) > 0.0f); // ビームが逆向きにならない。
        assert(std::abs(world.Determinant()) > 1.0f);    // 上下を向いても行列が壊れない。
    }

    const Vector3 fallback(0, 0, -1);
    assert(EyeTurretShape::DirectionTo(center, center, fallback) == fallback);
    assert(EyeTurretShape::BeamStart(center, center, fallback) == center);
    const Vector3 closeTarget = center + Vector3(0, 0, 2);
    assert(EyeTurretShape::BeamStart(center, closeTarget, Vector3(0, 0, 1)) == closeTarget);

    // 球面の全頂点は半径1、全三角形は有効で外向き。裏面カリングで消える面を作らない。
    const auto mesh = EyeTurretShape::BuildSphere();
    assert(mesh.positions.size() == 33 * 65);
    assert(mesh.indices.size() == 31 * 64 * 6);
    for (const auto& position : mesh.positions)
    {
        assert(std::abs(position.Length() - 1.0f) < 0.00001f);
    }
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            assert(mesh.indices[i + j] < mesh.positions.size());
        }
        const auto a = mesh.positions[mesh.indices[i]];
        const auto b = mesh.positions[mesh.indices[i + 1]];
        const auto c = mesh.positions[mesh.indices[i + 2]];
        const auto normal = (b - a).Cross(c - a);
        assert(normal.LengthSquared() > 0.00000001f);
        assert(normal.Dot(a + b + c) > 0.0f);
    }
    std::cout << "Eye turret tests passed: smooth rotation, FPS independence, sphere and beam alignment.\n";
}
