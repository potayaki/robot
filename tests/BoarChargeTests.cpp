#include "../DX22_01_plane/BoarCharge.h"
#include <cassert>
#include <iostream>

using DirectX::SimpleMath::Vector3;
using State = BoarCharge::State;

int main()
{
    BoarCharge charge;
    Vector3 position(0, 14, 0), target(0, 10, 150);
    charge.Update(0.5f, position, target, 170);
    assert(charge.GetState() == State::Windup && position.z == 0);
    assert(charge.Direction().z == 1 && charge.PreviewEnd().z == 240);

    // 予備動作中にPlayerが横へ逃げても、予測線と実際の突進方向は変わらない。
    target.x = 200;
    charge.Update(0.6f, position, target, 170);
    assert(charge.GetState() == State::Charging);
    assert(std::abs(position.z - 21) < 0.001f && position.x == 0);
    charge.MarkHit();
    assert(charge.HasHit());
    const auto movement = charge.Update(1.2f, position, target, 170);
    assert(movement.charged && movement.end.z == 240);
    assert(charge.GetState() == State::Recovery && position.x == 0);

    // 同じ実時間で、接近→予備動作→突進をまたいでもFPSに依存しない位置へ進む。
    Vector3 reference;
    for (int fps : {30, 60, 144})
    {
        BoarCharge timed;
        Vector3 location(0, 14, -100);
        for (int i = 0; i < fps * 4; ++i) timed.Update(1.0f / fps, location, Vector3(0, 10, 150), 170);
        assert(timed.GetState() == State::Charging);
        if (fps == 30) reference = location;
        else assert((location - reference).Length() < 0.01f);
    }

    BoarCharge shortRange;
    Vector3 farPosition(0, 14, 0);
    shortRange.Update(0.1f, farPosition, Vector3(0, 10, 150), 80);
    assert(shortRange.GetState() == State::Chase); // 開始距離を変えると実際の発動条件も変わる。
    const Vector3 paused = farPosition;
    shortRange.Update(0, farPosition, Vector3(0, 10, 150), 80);
    assert(farPosition == paused);
    std::cout << "Boar charge tests passed.\n";
}
