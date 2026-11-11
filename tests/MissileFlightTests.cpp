/// @file MissileFlightTests.cpp
/// @brief 動作と境界条件を検証する MissileFlightTests の自動テスト。

#include "../DX22_01_plane/Bezier.h"
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
