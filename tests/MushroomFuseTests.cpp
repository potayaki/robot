/// @file MushroomFuseTests.cpp
/// @brief 動作と境界条件を検証する MushroomFuseTests の自動テスト。

#include "../DX22_01_plane/MushroomFuse.h"
#include <cassert>
#include <cmath>

int main() {
    MushroomFuse hopping;
    assert(hopping.HopHeight() == 0);
    hopping.Update(0.35f, 100);
    // 現在のゲーム設定は高さ50。旧設定の10を期待していたテストを更新する。
    assert(std::abs(hopping.HopHeight() - 50.0f) < 0.001f);
    const float pausedHeight = hopping.HopHeight();
    hopping.Update(0, 100);
    assert(hopping.HopHeight() == pausedHeight);
    hopping.Update(0.35f, 100);
    assert(std::abs(hopping.HopHeight()) < 0.001f);
    hopping.Update(0.2f, 100);
    assert(hopping.HopHeight() > 0);
    hopping.Update(0.01f, 45);
    assert(hopping.Armed() && hopping.HopHeight() == 0);
    hopping.Update(0.4f, 500);
    assert(hopping.HopHeight() == 0);

    MushroomFuse fuse;
    assert(fuse.Update(1, 100) == 24 && !fuse.Armed());
    assert(fuse.Update(0.1f, 45) == 0 && fuse.Armed() && fuse.FlashRed());
    assert(fuse.Update(0.1f, 500) == 0 && !fuse.FlashRed());
    assert(fuse.Update(0, 500) == 0 && !fuse.Exploded());
    fuse.Update(1.3f, 500);
    assert(fuse.Exploded() && fuse.Update(1, 500) == 0);
    for (int fps : {30, 60, 144}) {
        MushroomFuse timed;
        float distance = 69;
        for (int frame = 0; frame < fps * 2; ++frame) {
            distance -= timed.Update(1.0f / fps, distance);
        }
        assert(std::abs(distance - 45) < 0.001f && !timed.Exploded());
        for (int frame = 0; frame < fps; ++frame) {
            timed.Update(1.0f / fps, 500);
        }
        assert(timed.Exploded());
    }
    MushroomFuse hitch;
    assert(hitch.Update(3, 69) == 24 && hitch.Exploded());
}
