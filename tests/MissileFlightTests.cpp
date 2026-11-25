/// @file MissileFlightTests.cpp
/// @brief 動作と境界条件を検証する MissileFlightTests の自動テスト。

#include "../DX22_01_plane/Bezier.h"
#include "../DX22_01_plane/CMissile.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using DirectX::SimpleMath::Vector3;

float Simulate(const Vector3& target, float frameSeconds, float spread = 0.0f) {
    Bezier flight;
    flight.Create(Vector3(0, 5, 0), target, spread);
    float elapsed = 0.0f;
    while (flight.IsActive() && elapsed < 20.0f) {
        flight.Update(frameSeconds);
        elapsed += frameSeconds;
    }

    assert(!flight.IsActive());
    assert(flight.GetTime() == 1.0f);
    assert((Vector3(flight.GetCulvePosition(1.0f)) - target).Length() < 0.0001f);
    assert(std::abs(elapsed - flight.GetFlightDurationSeconds()) <= frameSeconds + 0.0001f);
    return elapsed;
}

int main() {
    // 中心が奥にある大型の球でも、表面が先に軌道へ入ればその場で炸裂する。
    const Collision::Segment movement{{0, 0, 0}, {100, 0, 0}};
    const Collision::Sphere scout{{55, 0, 0}, 3};
    const Collision::Sphere boss{{70, 0, 0}, 30};
    for (const auto& spheres : {std::array{scout, boss}, std::array{boss, scout}}) {
        float first = 2.0f;
        for (const auto& sphere : spheres) {
            float fraction = 0.0f;
            assert(Collision::SegmentSphereHitFraction(movement, sphere, 2, fraction));
            first = (std::min)(first, fraction);
        }
        assert(std::abs(first - 0.38f) < 0.00001f);
    }
    float fraction = 2.0f;
    const Collision::Segment stationary{movement.start, movement.start};
    assert(!Collision::SegmentSphereHitFraction(stationary, boss, 2, fraction));
    assert(Collision::SegmentSphereHitFraction(stationary, {movement.start, 10}, 2, fraction));
    assert(fraction == 0.0f);
    assert(Collision::SegmentSphereHitFraction(movement, {{5, 0, 0}, 10}, 2, fraction));
    assert(fraction == 0.0f);
    assert(Collision::SegmentSphereHitFraction(movement, {{112, 0, 0}, 10}, 2, fraction));
    assert(fraction == 1.0f);
    assert(Collision::SegmentSphereHitFraction(movement, {{50, 12, 0}, 10}, 2, fraction));
    assert(std::abs(fraction - 0.5f) < 0.00001f);
    assert(!Collision::SegmentSphereHitFraction(movement, {{50, 13, 0}, 10}, 2, fraction));

    // 敵の外周までの3D距離で、中心・減衰帯・範囲外を区別する。強化後の威力も反映する。
    const Vector3 impact(30, 20, 100);
    assert(CMissile::BlastDamage(3, impact, {impact, 10}) == 3);
    assert(CMissile::BlastDamage(3, impact, {impact + Vector3(46, 0, 18), 10}) == 3);
    assert(CMissile::BlastDamage(5, impact, {impact + Vector3(0, 75, 0), 10}) == 5);
    assert(CMissile::BlastDamage(5, impact, {impact + Vector3(0, 76, 0), 10}) == 3);
    assert(CMissile::BlastDamage(4, impact, {impact + Vector3(105, 0, 0), 10}) == 2);
    assert(CMissile::BlastDamage(4, impact, {impact + Vector3(0, 106, 0), 10}) == 0);
    assert(CMissile::BlastDamage(0, impact, {impact, 10}) == 0);
    // 威力1なら内外のダメージ値が一致しても、デバッグ表示は実際に重なった帯を区別する。
    bool fullHit;
    assert(CMissile::BlastDamage(1, impact, {impact + Vector3(75, 0, 0), 10}, 1.0f, &fullHit) == 1 &&
           fullHit);
    assert(CMissile::BlastDamage(1, impact, {impact + Vector3(76, 0, 0), 10}, 1.0f, &fullHit) == 1 &&
           !fullHit);
    assert(CMissile::BlastDamage(1, impact, {impact + Vector3(106, 0, 0), 10}, 1.0f, &fullHit) == 0 &&
           !fullHit);

    // 爆風拡張は全ダメージ帯と半減帯の両方を広げる。敵の半径やダメージ値は変えない。
    constexpr std::array expectedScales{1.0f, 1.15f, 1.30f, 1.45f};
    for (int rank = 0; rank <= 3; ++rank) {
        const float scale = UpgradeBalance::MissileBlastScale(rank);
        assert(std::abs(scale - expectedScales[rank]) < 0.00001f);
        const float inner = 65.0f * expectedScales[rank] + 10.0f;
        const float outer = 95.0f * expectedScales[rank] + 10.0f;
        const auto damageAt = [&](float distance) {
            return CMissile::BlastDamage(5, impact, {impact + Vector3(0, distance, 0), 10}, scale);
        };
        assert(damageAt(inner - 0.01f) == 5);
        assert(damageAt(inner + 0.01f) == 3);
        assert(damageAt(outer - 0.01f) == 3);
        assert(damageAt(outer + 0.01f) == 0);
    }

    // 地上・空中とも距離が伸びるほど到達時間が長くなり、FPSによる差は1フレーム以内。
    for (float spread : {0.0f, 45.0f, -90.0f}) {
        for (int fps : {30, 60, 144}) {
            const float dt = 1.0f / fps;
            const float nearTime = Simulate(Vector3(0, 5, 100), dt, spread);
            const float middleTime = Simulate(Vector3(0, 5, 600), dt, spread);
            const float farTime = Simulate(Vector3(0, 5, 1800), dt, spread);
            assert(nearTime < 0.35f);
            assert(nearTime < middleTime && middleTime < farTime);
            assert(farTime >= 3.0f - dt);
            assert(Simulate(Vector3(0, 45, 100), dt, spread) < Simulate(Vector3(0, 125, 1800), dt, spread));
        }
    }

    Bezier flight;
    flight.Create(Vector3(0, 0, 0), Vector3(0, 0, 600), 0.0f);
    flight.Update(0.0f);
    flight.Update(-1.0f);
    assert(flight.GetTime() == 0.0f);
    flight.Update(0.2f);
    const float progress = flight.GetTime();
    const float duration = flight.GetFlightDurationSeconds();
    flight.UpdateTargetPosition(Vector3(30, 50, 650));
    assert(flight.GetTime() == progress);
    assert(flight.GetFlightDurationSeconds() == duration);
    flight.Update(10.0f);
    assert(!flight.IsActive());
    assert((Vector3(flight.GetCulvePosition(flight.GetTime())) - Vector3(30, 50, 650)).Length() < 0.0001f);

    // プール再使用時には飛行時間を近距離用に再計算し、進行度も初期化する。
    flight.Create(Vector3(0, 0, 0), Vector3(0, 0, 50), 0.0f);
    assert(flight.IsActive() && flight.GetTime() == 0.0f);
    assert(flight.GetFlightDurationSeconds() < duration);
    Simulate(Vector3(0, 5, 0), 1.0f / 60.0f); // 距離0でも有限時間で終了する。

    std::cout << "Missile flight tests passed: distance, spread, airborne targets, FPS and reuse.\n";
    for (float distance : {100.0f, 600.0f, 1800.0f}) {
        flight.Create(Vector3(0, 0, 0), Vector3(0, 0, distance), 0.0f);
        std::cout << "Distance " << distance << ": " << flight.GetFlightDurationSeconds() << " seconds\n";
    }
}
