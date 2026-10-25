#include "../DX22_01_plane/BulletHoming.h"
#include <SimpleMath.h>
#include <cassert>
#include <iostream>

using DirectX::SimpleMath::Vector3;

namespace
{
    float AngleDegrees(Vector3 a, Vector3 b)
    {
        a.Normalize();
        b.Normalize();
        return std::acos(std::clamp(a.Dot(b), -1.0f, 1.0f)) * 180.0f / 3.14159265f;
    }
} // namespace

int main()
{
    const Vector3 zero(0, 0, 0);
    const Vector3 velocity(0, 0, 25);
    const Vector3 target(100, 60, 200);
    // 未取得は直進。レベルごとの旋回上限・弾速維持・高さ方向の誘導を確認。
    assert(BulletHoming::Turn(velocity, target, 0, 1.0f / 60.0f) == velocity);
    float previousAngle = 0;
    for (int rank = 1; rank <= BulletHoming::MaxRank; ++rank)
    {
        const auto turned = BulletHoming::Turn(velocity, target, rank, 1.0f / 60.0f);
        const float angle = AngleDegrees(velocity, turned);
        assert(angle > previousAngle);
        assert(std::abs(angle - BulletHoming::DegreesPerSecond(rank) / 60.0f) < 0.001f);
        assert(std::abs(turned.Length() - 25.0f) < 0.0001f);
        assert(turned.x > 0 && turned.y > 0);
        previousAngle = angle;
    }
    assert(BulletHoming::Turn(velocity, Vector3(0, 0, -10), 5, 1.0f / 60.0f) == velocity);
    assert(BulletHoming::Turn(velocity, zero, 5, 1.0f / 60.0f) == velocity);
    assert(BulletHoming::Turn(zero, target, 5, 1.0f / 60.0f) == zero);
    const auto almostStraight = BulletHoming::Turn(velocity, Vector3(0.1f, 0, 100), 5, 1.0f / 60.0f);
    assert(AngleDegrees(almostStraight, Vector3(0.1f, 0, 100)) < 0.02f);

    // 弾の前方・範囲内だけから最短距離を選ぶ。プレイヤー位置には依存しない。
    BulletHoming::Search<Vector3> search(velocity);
    search.Consider(Vector3(0, 0, -1));
    search.Consider(Vector3(0, 0, 601));
    assert(!search.Found());
    search.Consider(Vector3(0, 0, 400));
    search.Consider(Vector3(20, 0, 80));
    search.Consider(Vector3(10, 0, 200));
    assert(search.Found() && search.Offset() == Vector3(20, 0, 80));

    // 弱い誘導は横の敵に届かず、その後ろにいる別の敵へ再誘導する。
    Vector3 position = zero;
    Vector3 moving = velocity;
    const Vector3 firstEnemy(35, 0, 80);
    const Vector3 secondEnemy(0, 0, 300);
    bool retargeted = false;
    float nearestFirst = 1000;
    for (int frame = 0; frame < 12; ++frame)
    {
        BulletHoming::Search<Vector3> step(moving);
        step.Consider(firstEnemy - position);
        step.Consider(secondEnemy - position);
        if (position.z > firstEnemy.z && step.Found() && step.Offset() == secondEnemy - position)
        {
            retargeted = true;
        }
        if (step.Found())
        {
            moving = BulletHoming::Turn(moving, step.Offset(), 1, 1.0f / 60.0f);
        }
        position += moving;
        nearestFirst = (std::min)(nearestFirst, (firstEnemy - position).Length());
    }
    assert(nearestFirst > 10 && retargeted);
    // 敵消失・全候補通過時は標的を保持せず、そのまま直進できる。
    BulletHoming::Search<Vector3> empty(moving);
    assert(!empty.Found());
    // 同じ0.25秒ならFPSが違っても同じ旋回角・同じ弾速になる。
    for (int fps : {30, 60, 120, 144})
    {
        Vector3 heading = velocity;
        const float dt = 0.25f / (fps / 4);
        for (int i = 0; i < fps / 4; ++i)
        {
            heading = BulletHoming::Turn(heading, Vector3(1000, 0, 1000), 1, dt);
        }
        assert(std::abs(AngleDegrees(velocity, heading) - 6.0f) < 0.002f);
        assert(std::abs(heading.Length() - velocity.Length()) < 0.0001f);
    }
    assert(BulletHoming::Turn(velocity, target, 5, 0.0f) == velocity);
    assert(BulletHoming::Turn(velocity, target, 5, -0.1f) == velocity);
    std::cout << "Bullet homing tests passed.\n";
}
