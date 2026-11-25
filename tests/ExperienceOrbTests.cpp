/// @brief 経験値の粒の着地、待機、追尾と、到着前に報酬が発生しないことを確認する。
#include "../DX22_01_plane/ExperienceOrb.h"
#include "../DX22_01_plane/ExperienceOrbDrop.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

using DirectX::SimpleMath::Vector3;

namespace {
    void CheckDropRewards() {
        std::mt19937 random(20261124);
        std::array<bool, 3> countsSeen{};
        std::array<bool, 4> rewardsSeen{};
        std::vector<ExperienceOrb> orbs;
        const Vector3 origin(10, 32, -15);
        for (const int weight : {1, 2, 15, 45}) {
            for (int roll = 0; roll < 128; ++roll) {
                orbs.clear();
                AppendExperienceOrbDrop(orbs, origin, 0, weight, random);
                assert(orbs.size() >= 8 && orbs.size() <= 10);
                if (weight == 1) {
                    countsSeen[orbs.size() - 8] = true;
                }
                int total = 0;
                for (const auto& orb : orbs) {
                    assert(orb.position == origin);
                    assert(orb.experience >= 2 && orb.experience <= 5);
                    if (weight == 1) {
                        rewardsSeen[orb.experience - 2] = true;
                    }
                    total += orb.experience;
                }
                assert(total >= 16 && total <= 50);
            }
        }
        assert(std::all_of(countsSeen.begin(), countsSeen.end(), [](bool seen) { return seen; }));
        assert(std::all_of(rewardsSeen.begin(), rewardsSeen.end(), [](bool seen) { return seen; }));

        // 先の撃破で生じた粒はそのまま。報酬のない退場は1個も増やさない。
        const auto previous = orbs;
        const Vector3 other(-60, 50, -90);
        for (const int weight : {0, -1}) {
            AppendExperienceOrbDrop(orbs, other, 0, weight, random);
            assert(orbs.size() == previous.size());
        }
        AppendExperienceOrbDrop(orbs, other, 0, 1, random);
        const auto added = orbs.size() - previous.size();
        assert(added >= 8 && added <= 10);
        for (size_t i = 0; i < previous.size(); ++i) {
            assert(orbs[i].position == previous[i].position);
            assert(orbs[i].experience == previous[i].experience);
            assert(orbs[i].phase == previous[i].phase);
        }
        for (size_t i = previous.size(); i < orbs.size(); ++i) {
            assert(orbs[i].position == other);
            for (int frame = 0; frame < 120; ++frame) {
                assert(!orbs[i].Advance(1.0f / 60, other + Vector3(200, 0, 0),
                                        [](float, float) { return 0.0f; }));
            }
            const Vector3 spread = orbs[i].position - other;
            const float radius = std::hypot(spread.x, spread.z);
            assert(radius > 5.0f && radius < 18.0f);
            for (size_t j = previous.size(); j < i; ++j) {
                assert(Vector3::Distance(orbs[i].position, orbs[j].position) > 0.5f);
            }
        }
    }

    void CheckDistantDropAndPickup(int fps) {
        const float step = 1.0f / fps;
        constexpr float groundY = 26.0f;
        const auto heightAt = [](float, float) { return groundY; };
        const Vector3 origin(13, 78, -32);
        ExperienceOrb orb(origin, groundY, 15, 0.0f, {0, 39, 0});
        Vector3 receiver = origin + Vector3(ExperienceOrb::MagnetRadius + 1.0f, 0, 0);
        assert(!orb.Advance(step, receiver, heightAt) && orb.position.y > origin.y);
        // 高い場所から落とした粒も地面まで降り、遠くにいる間は消えない。
        for (int i = 0; i < fps * 20; ++i) {
            assert(!orb.Advance(step, receiver, heightAt));
        }
        assert(orb.position == Vector3(origin.x, groundY + ExperienceOrb::HoverHeight, origin.z));
        assert(!orb.IsAttracting() && orb.experience == 15);
        // 境界に入ると吸引が始まる。届くまで経験値を受け取らず、外に走り出しても追ってくる。
        receiver.x = origin.x + ExperienceOrb::MagnetRadius;
        assert(!orb.Advance(step, receiver, heightAt) && orb.IsAttracting());
        const Vector3 paused = orb.position;
        assert(!orb.Advance(0, receiver, heightAt) && orb.position == paused);
        int frames = 0;
        while (frames < fps * 2) {
            receiver.x += 80.0f * step;
            ++frames;
            if (orb.Advance(step, receiver, heightAt)) {
                break;
            }
        }
        assert(frames < fps * 2 && frames > fps / 3);
        assert(orb.position == receiver);
    }

    float CheckCloseDrop(int fps) {
        const float step = 1.0f / fps;
        const Vector3 receiver(0, 12, 0);
        ExperienceOrb orb({0, 2, 0}, 0, 1, 0.0f, {0, 39, 0});
        const auto flatGround = [](float, float) { return 0.0f; };
        for (int frame = 1; frame <= fps; ++frame) {
            if (orb.Advance(step, receiver, flatGround)) {
                // 足元で撃破しても、跳ねて落ちる演出を経てから回収する。
                const float time = static_cast<float>(frame) / fps;
                assert(time > 0.4f && time < 0.65f);
                assert(orb.position == receiver);
                return time;
            }
        }
        assert(false);
        return 0;
    }

    void CheckScatterAndPartialPickup(int fps) {
        const auto slope = [](float x, float z) { return 0.22f * x - 0.17f * z; };
        const float step = 1.0f / fps;
        const Vector3 origin(0, ExperienceOrb::HoverHeight, 0);
        ExperienceOrb toward(origin, 0, 1, 0, {20, 40, 15});
        ExperienceOrb away(origin, 0, 1, 1, {-40, 52, -30});
        assert(toward.position == away.position);
        const Vector3 distant(200, 14, 0);
        for (int i = 0; i < fps * 2; ++i) {
            assert(!toward.Advance(step, distant, slope));
            assert(!away.Advance(step, distant, slope));
        }
        const Vector3 towardRest = toward.position;
        const Vector3 awayRest = away.position;
        for (const auto& resting : {towardRest, awayRest}) {
            const float radius = std::hypot(resting.x, resting.z);
            assert(radius > 5.0f && radius < 18.0f);
            assert(std::abs(resting.y - slope(resting.x, resting.z) - ExperienceOrb::HoverHeight) < 0.0001f);
        }
        assert(towardRest.x > 0 && towardRest.z > 0 && awayRest.x < 0 && awayRest.z < 0);

        const Vector3 receiver(ExperienceOrb::MagnetRadius, 12, 0);
        bool collected = false;
        for (int i = 0; i < fps * 2 && !collected; ++i) {
            collected = toward.Advance(step, receiver, slope);
            assert(!away.Advance(step, receiver, slope));
        }
        assert(collected && !away.IsAttracting() && away.position == awayRest);
        assert(!away.Advance(step, awayRest + Vector3(8, 4, 0), slope) && away.IsAttracting());
    }

    Vector3 CheckAirborneScatter(int fps) {
        const auto flatGround = [](float, float) { return 0.0f; };
        ExperienceOrb orb({0, 1200, 0}, 0, 1, 0, {30, 56, -40});
        const Vector3 receiver(300, 0, 0);
        const Vector3 origin = orb.position;
        assert(!orb.Advance(0, receiver, flatGround) && orb.position == origin);
        for (int i = 0; i < fps * 5; ++i) {
            assert(!orb.Advance(1.0f / fps, receiver, flatGround));
        }
        const float radius = std::hypot(orb.position.x, orb.position.z);
        assert(radius > 17.5f && radius < 18.0f);
        assert(orb.position.y == ExperienceOrb::HoverHeight);
        return orb.position;
    }
} // namespace

int main() {
    CheckDropRewards();
    const float referenceTime = CheckCloseDrop(60);
    const auto airborne = CheckAirborneScatter(60);
    for (const int fps : {30, 60, 144}) {
        CheckDistantDropAndPickup(fps);
        CheckScatterAndPartialPickup(fps);
        assert(Vector3::Distance(airborne, CheckAirborneScatter(fps)) < 0.01f);
        assert(std::abs(CheckCloseDrop(fps) - referenceTime) <= 1.0f / 30);
    }
    std::cout
        << "Experience orbs: drop rewards, scatter, landing, attraction, pause and pickup timing passed.\n";
}
