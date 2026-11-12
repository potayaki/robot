/// @file GhostSupportTests.cpp
/// @brief 支援上限・範囲・死亡時解除・整数ダメージ・FPSによらない間合いを検証。
#include "../DX22_01_plane/GhostSupportRules.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace GhostSupport;
    Settings settings;
    std::vector<Candidate> enemies{{0, 400, true}, {1, 100, true}, {2, 900, true},
                                   {3, 625, true}, {4, 1, false},  {5, 301 * 301, true}};
    assert((Select(enemies, settings, true) == std::vector<size_t>{1, 0, 3}));
    settings.targetCount = 1;
    assert((Select(enemies, settings, true) == std::vector<size_t>{1}));
    settings.targetCount = 10;
    assert(Select(enemies, settings, true).size() == 4);
    assert(Select(enemies, settings, false).empty()); // 支援元死亡で全解除。
    settings.targetCount = 0;
    assert(Select(enemies, settings, true).empty());
    settings = Settings{};
    enemies[1].eligible = false; // 一番近い仲間が死亡・無効になった場合。
    assert((Select(enemies, settings, true) == std::vector<size_t>{0, 3, 2}));
    enemies[0].distanceSquared = 400 * 400; // 範囲外への移動。
    assert((Select(enemies, settings, true) == std::vector<size_t>{3, 2}));
    assert(Select({{0, 300 * 300, true}}, settings, true).size() == 1);
    assert(Select({}, settings, true).empty());
    assert((Select({{0, 100, true}, {1, 100, true}}, settings, true) == std::vector<size_t>{0, 1}));
    assert(Incoming(10, settings.incomingMultiplier) == 7);
    assert(Incoming(2, settings.incomingMultiplier) == 1);
    assert(Incoming(1, settings.incomingMultiplier) == 1);
    assert(Incoming(0, settings.incomingMultiplier) == 0);
    assert(Outgoing(8, settings.outgoingMultiplier) == 12);
    assert(Outgoing(3, settings.outgoingMultiplier) == 5);
    assert(Outgoing(0, settings.outgoingMultiplier) == 0);
    for (int fps : {15, 30, 60, 144}) {
        for (float initial : {0.0f, 100.0f, 220.0f, 400.0f}) {
            float distance = initial;
            for (int frame = 0; frame < fps * 5; ++frame) {
                distance -= Travel(distance, 65, 1.0f / fps, 180, 260);
            }
            assert(std::abs(distance - std::clamp(initial, 180.0f, 260.0f)) < 0.01f);
        }
    }
    assert(Travel(100, 65, 0, 180, 260) == 0);
    assert(Travel(100, 65, 10, 180, 260) == -80);
    std::cout << "Ghost support and movement tests passed.\n";
}
