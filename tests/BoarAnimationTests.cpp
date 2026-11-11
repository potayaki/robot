/// @file BoarAnimationTests.cpp
/// @brief 動作と境界条件を検証する BoarAnimationTests の自動テスト。

#include "../DX22_01_plane/BoarAnimationRig.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    BoarAnimationRig rig;
    std::string error;
    if (!rig.Load("DX22_01_plane/assets/model/Boar/Boar.glb", error)) {
        std::cerr << error << '\n';
        return 1;
    }
    assert(!rig.GetVertices().empty());
    assert(!rig.GetMatrices().empty() && rig.GetMatrices().size() <= 64);
    assert(rig.GetAlbedo().size() > 8 && rig.GetAlbedo()[0] == 137 && rig.GetAlbedo()[1] == 'P');
    for (const auto& vertex : rig.GetVertices()) {
        float total = 0;
        for (unsigned i = 0; i < 4; ++i) {
            assert(vertex.bones[i] < rig.GetMatrices().size());
            total += vertex.weights[i];
        }
        assert(std::abs(total - 1) < 0.0001f);
    }
    const auto idle = rig.GetBounds();
    std::cout << "Idle bounds: " << idle.min.x << ',' << idle.min.y << ',' << idle.min.z << " to "
              << idle.max.x << ',' << idle.max.y << ',' << idle.max.z << '\n';
    assert(idle.max.y - idle.min.y > 2 && idle.max.y - idle.min.y < 5);

    rig.Update(BoarMotion::Death, 1.5f);
    assert(rig.DeathFinished() && rig.GetFrame() == 89);
    const auto death = rig.GetBounds();
    std::cout << "Death bounds: " << death.min.x << ',' << death.min.y << ',' << death.min.z << " to "
              << death.max.x << ',' << death.max.y << ',' << death.max.z << '\n';
    rig.Update(BoarMotion::Death, 20);
    assert(rig.GetFrame() == 89); // 死亡後に走行区間へ飛び、起き上がることを防ぐ。
    assert((rig.GetBounds().min - death.min).Length() < 0.0001f);

    for (int fps : {30, 60, 144}) {
        BoarAnimationRig run;
        assert(run.Load("DX22_01_plane/assets/model/Boar/Boar.glb", error));
        for (int i = 0; i < fps * 2; ++i) {
            run.Update(BoarMotion::Run, 1.0f / fps);
        }
        assert(std::abs(run.GetFrame() - 109.0) < 0.001);
        assert((run.GetBounds().max - idle.max).Length() > 0.01f);
    }
    std::cout << "Boar animation tests passed.\n";
}
