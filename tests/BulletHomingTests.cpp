#include "../DX22_01_plane/BulletHoming.h"
#include <SimpleMath.h>
#include "../DX22_01_plane/AimCursor.h"
#include <cassert>
#include <iostream>

using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector2;

namespace
{
    float AngleDegrees(Vector3 a, Vector3 b)
    {
        a.Normalize();
        b.Normalize();
        // 小さな旋回ではacosだと1付近の丸め誤差が増えるため、外積と内積から測る。
        return std::atan2(a.Cross(b).Length(), a.Dot(b)) * 180.0f / 3.14159265f;
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

    // 弾の前方・射程内から、ワールド距離ではなく照準中心からの距離を優先する。
    BulletHoming::Search<Vector3> search(velocity);
    search.Consider(Vector3(0, 0, -1), 0);
    search.Consider(Vector3(0, 0, 601), 0);
    assert(!search.Found());
    search.Consider(Vector3(0, 0, 400), 100);
    search.Consider(Vector3(20, 0, 80), 25);
    search.Consider(Vector3(10, 0, 200), 4);
    assert(search.Found() && search.Offset() == Vector3(10, 0, 200));
    // 画面上で同じ距離のときだけ、弾から近い敵を優先する。
    search.Consider(Vector3(5, 0, 130), 4);
    search.Consider(Vector3(0, 0, 1), -1);
    assert(search.Offset() == Vector3(5, 0, 130));

    // 回帰確認：近い横の敵より、画面上で照準の中心に近い奥の敵へ曲がる。
    const AimCursor::Region cursor(Vector3(0, 0, -100), zero, Vector2(0, 0), 720);
    const Vector3 outside(-30, 0, 30);
    const Vector3 insideNear(5, 0, 150);
    const Vector3 insideFar(-2, 0, 350);
    const Vector3 candidates[] = { outside, insideFar, insideNear };
    auto selectInCursor = [&](const AimCursor::Region& region, const Vector3& bulletPosition)
    {
        BulletHoming::Search<Vector3> selection(velocity);
        for (const auto& center : candidates)
        {
            float score = 0;
            if (region.TryGetBulletScore(center, 5, score)) selection.Consider(center - bulletPosition, score);
        }
        return selection;
    };
    const auto aimed = selectInCursor(cursor, zero);
    assert(!cursor.Contains(outside, 2.0f));
    assert(aimed.Found() && aimed.Offset() == insideFar);
    assert(BulletHoming::Turn(velocity, aimed.Offset(), 1, 1.0f / 60.0f).x < 0.0f);

    // 手前を通過した後は、同じ円内に残っている奥の敵へ切り替えられる。
    const Vector3 passedPosition(0, 0, 200);
    const auto passed = selectInCursor(cursor, passedPosition);
    assert(passed.Found() && passed.Offset() == insideFar - passedPosition);

    // 現在のカーソルが別方向へ動いたら、以前の敵への固定ロックを残さない。
    const AimCursor::Region movedCursor(Vector3(0, 0, -100), zero, Vector2(500, 200), 720);
    assert(!selectInCursor(movedCursor, zero).Found());

    // 画面端でも、水平・垂直の境界が表示円と一致する（円外の四隅も除く）。
    const float focalLength = 360.0f / std::tan(AimCursor::FieldOfView * 0.5f);
    const auto screenPoint = [&](float x, float y)
    {
        return Vector3(x * 500.0f / focalLength, y * 500.0f / focalLength, 500.0f);
    };
    const AimCursor::Region edgeCursor(zero, Vector3(0, 0, 1), Vector2(500, 200), 720);
    // Lv0は中心を狙っても誘導なし。Lv1～5では表示サイズと枠内判定が同じ比率で広がる。
    float score = 0;
    assert(!edgeCursor.TryGetBulletScore(screenPoint(500, 200), 0, score));
    float previousRadius = 0;
    for (int rank = 1; rank <= BulletHoming::MaxRank; ++rank)
    {
        assert(BulletHoming::CrosshairSize(rank) == 32.0f + 16.0f * (rank - 1));
        const float bulletRadius = BulletHoming::CursorRadius(rank);
        assert(bulletRadius > previousRadius);
        assert(edgeCursor.TryGetBulletScore(screenPoint(500 + bulletRadius - 0.1f, 200), rank, score));
        assert(!edgeCursor.TryGetBulletScore(screenPoint(500 + bulletRadius + 0.1f, 200), rank, score));
        previousRadius = bulletRadius;
    }
    assert(!edgeCursor.TryGetBulletScore(screenPoint(520, 200), 1, score));
    assert(edgeCursor.TryGetBulletScore(screenPoint(520, 200), 2, score));
    assert(std::abs(score - 400.0f) < 0.01f);

    // 大きいミサイルカーソルは、通常弾のレベルとは独立して従来の範囲を使う。
    const float radius = AimCursor::MissileRadius;
    assert(edgeCursor.Contains(screenPoint(500, 200), 0));
    assert(edgeCursor.Contains(screenPoint(500 + radius - 1, 200), 0));
    assert(!edgeCursor.Contains(screenPoint(500 + radius + 1, 200), 0));
    assert(edgeCursor.Contains(screenPoint(500, 200 - radius + 1), 0));
    assert(!edgeCursor.Contains(screenPoint(500, 200 - radius - 1), 0));
    assert(!edgeCursor.Contains(screenPoint(500 + radius, 200 + radius), 0));

    // 大型ボスの体を直接狙う補助と、カメラ背後・無効なカメラの除外。
    const AimCursor::Region centerCursor(zero, Vector3(0, 0, 1), Vector2(0, 0), 720);
    assert(centerCursor.Contains(Vector3(100, 0, 300), 101));
    assert(!centerCursor.Contains(Vector3(100, 0, 300), 99));
    assert(!centerCursor.Contains(Vector3(0, 0, -10), 20));
    assert(!AimCursor::Region(zero, zero, Vector2(0, 0), 720).Contains(Vector3(0, 0, 10), 1));
    const AimCursor::Region rotatedCursor(Vector3(100, 25, 40), Vector3(0, 25, 40), Vector2(0, 0), 720);
    assert(rotatedCursor.Contains(Vector3(-200, 25, 40), 1));
    assert(!rotatedCursor.Contains(Vector3(200, 25, 40), 1));

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
        step.Consider(firstEnemy - position, 4);
        step.Consider(secondEnemy - position, 9);
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
        assert(std::abs(AngleDegrees(velocity, heading) - BulletHoming::DegreesPerSecond(1) * 0.25f) < 0.002f);
        assert(std::abs(heading.Length() - velocity.Length()) < 0.0001f);
    }
    assert(BulletHoming::Turn(velocity, target, 5, 0.0f) == velocity);
    assert(BulletHoming::Turn(velocity, target, 5, -0.1f) == velocity);
    std::cout << "Bullet homing tests passed.\n";
}
