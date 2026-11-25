/// @file RobotSpiderIKTests.cpp
/// @brief 動作と境界条件を検証する RobotSpiderIKTests の自動テスト。

#include "../DX22_01_plane/RobotSpiderIK.h"
#include "../DX22_01_plane/RobotDebris.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

constexpr int LegJoints[][4] = {{3, 4, 5, 6},     {9, 10, 11, 12},  {15, 16, 17, 18},
                                {27, 28, 29, 30}, {33, 34, 35, 36}, {39, 40, 41, 42}};

aiMatrix4x4 World(aiVector3D position, float yaw = 0) {
    return aiMatrix4x4(aiVector3D(0.025f), aiQuaternion(aiVector3D(0, 1, 0), yaw), position);
}

void CheckSupportWeightShift(const RobotAnimationRig& original) {
    const float dt = 1.0f / 60;
    const auto basis = aiMatrix3x3(World(aiVector3D(), 0.7f));
    const auto forward = (basis * aiVector3D(0, 0, 1)).Normalize();
    const auto lateral = (basis * aiVector3D(1, 0, 0)).Normalize();
    for (float speed : {60.0f, -60.0f}) {
        auto rig = original;
        RobotSpiderIK ik;
        assert(ik.Init(rig));
        aiVector3D position;
        bool shiftedLeft = false, shiftedRight = false;
        for (int frame = 0; frame < 180; ++frame) {
            position += forward * (speed * dt);
            ik.Update(rig, dt, World(position, 0.7f), 0, true);
            const auto& feet = ik.GetFeet();
            const auto swing = std::find_if(feet.begin(), feet.end(), [](const auto& foot) {
                return foot.swinging && foot.progress > 0.8f;
            });
            if (frame > 60 && swing != feet.end()) {
                float support = 0.0f;
                for (const auto& foot : feet) {
                    if (!foot.swinging) {
                        support += (foot.position - position) * lateral;
                    }
                }
                const float shift = ik.GetBodyOffset() * lateral;
                shiftedLeft |= support < 0.0f && shift < -0.10f;
                shiftedRight |= support > 0.0f && shift > 0.10f;
            }
        }
        assert(shiftedLeft && shiftedRight);
        for (int frame = 0; frame < 60; ++frame) {
            ik.Update(rig, dt, World(position, 0.7f), 0, true);
        }
        assert(std::abs(ik.GetBodyOffset() * lateral) < 0.001f);
    }
}

void CheckStrideAndStopping(const RobotAnimationRig& original) {
    CheckSupportWeightShift(original);
    for (int fps : {30, 60, 144}) {
        const float dt = 1.0f / fps;
        // 足を上げた直後・途中・着地直前のいずれで操作を離しても、Idleへ戻らない。
        for (int phase = 0; phase < 12; ++phase) {
            auto rig = original;
            RobotSpiderIK ik;
            assert(ik.Init(rig));
            aiVector3D position(0, -5, 0);
            ik.Update(rig, dt, World(position), -5, true);
            const int frames = fps * 2 + phase * fps / 60;
            for (int frame = 0; frame < frames; ++frame) {
                position.z += 60.0f * dt;
                rig.Update(RobotMotion::Idle, dt);
                ik.Update(rig, dt, World(position), -5, true);
            }
            const auto stopped = ik.GetFeet();
            for (int frame = 0; frame < fps; ++frame) {
                rig.Update(RobotMotion::Idle, dt);
                ik.Update(rig, dt, World(position), -5, true);
                for (size_t i = 0; i < stopped.size(); ++i) {
                    const auto& foot = ik.GetFeet()[i];
                    assert(foot.stepNumber == stopped[i].stepNumber);
                    assert(std::abs(foot.position.x - stopped[i].position.x) < 0.001f);
                    assert(std::abs(foot.position.z - stopped[i].position.z) < 0.001f);
                    assert((ik.GetSolvedContacts()[i] - foot.position).Length() < 0.15f);
                    if (frame > fps / 4) {
                        assert(!foot.swinging);
                        assert(std::abs(foot.position.y + 5.0f) < 0.001f);
                    }
                }
            }
        }

        auto rig = original;
        RobotSpiderIK ik;
        assert(ik.Init(rig));
        aiVector3D position(0, -5, 0);
        ik.Update(rig, dt, World(position), -5, true);
        float totalStride = 0.0f;
        unsigned steps = 0;
        for (int frame = 0; frame < fps * 5; ++frame) {
            const auto previous = ik.GetFeet();
            position.z += 60.0f * dt;
            rig.Update(RobotMotion::Idle, dt);
            ik.Update(rig, dt, World(position), -5, true);
            if (frame < fps) {
                continue;
            }
            for (size_t i = 0; i < previous.size(); ++i) {
                const auto& foot = ik.GetFeet()[i];
                if (foot.stepNumber == previous[i].stepNumber) {
                    continue;
                }
                totalStride += (foot.target - foot.start).Length();
                ++steps;
            }
        }
        const float averageStride = totalStride / steps;
        const float stepsPerSecondPerFoot = steps / (4.0f * 6.0f);
        std::cout << fps << "fps stride=" << averageStride << " steps/foot/second=" << stepsPerSecondPerFoot
                  << '\n';
        assert(averageStride >= 19.0f && averageStride <= 21.0f);
        assert(stepsPerSecondPerFoot > 2.5f && stepsPerSecondPerFoot < 3.3f);
    }
}

void CheckDiagonalReversals(const RobotAnimationRig& original) {
    const aiVector3D diagonals[] = {{1, 0, 1}, {-1, 0, 1}, {-1, 0, -1}, {1, 0, -1}};
    for (int fps : {30, 60, 144}) {
        const float dt = 1.0f / fps;
        float maxError = 0.0f, maxSlide = 0.0f;
        const auto advance = [&](RobotSpiderIK& ik, RobotAnimationRig& rig, const aiVector3D& position) {
            const auto previousFeet = ik.GetFeet();
            const auto previousContacts = ik.GetSolvedContacts();
            ik.Update(rig, dt, World(position), -5, true);
            maxError = (std::max)(maxError, ik.GetContactError());
            int support = 0;
            for (size_t i = 0; i < previousFeet.size(); ++i) {
                const auto& foot = ik.GetFeet()[i];
                support += !foot.swinging;
                if (!foot.swinging && !previousFeet[i].swinging &&
                    foot.stepNumber == previousFeet[i].stepNumber) {
                    const auto slide = (ik.GetSolvedContacts()[i] - previousContacts[i]).Length();
                    maxSlide = (std::max)(maxSlide, slide);
                }
            }
            assert(support >= 3);
            return support;
        };
        for (auto direction : diagonals) {
            const auto frameTravel = direction.Normalize() * (60.0f * dt);
            // 一周期（歩幅20 / 速度60）を6等分し、両方の3本組の一歩の途中で反転する。
            for (int phase = 0; phase < 6; ++phase) {
                const int turnFrame = fps + phase * fps / 18;
                auto turnRig = original;
                RobotSpiderIK turnIK;
                assert(turnIK.Init(turnRig));
                aiVector3D turnPosition(0, -5, 0);
                turnIK.Update(turnRig, dt, World(turnPosition), -5, true);
                for (int frame = 0; frame < turnFrame; ++frame) {
                    turnPosition += frameTravel;
                    advance(turnIK, turnRig, turnPosition);
                }
                // 同じ歩行姿勢から、即反転・1フレーム停止・全脚が着地してからの反転を比べる。
                const int fullStopFrames = fps / 5;
                for (int pauseFrames : {0, 1, fullStopFrames}) {
                    auto rig = turnRig;
                    auto ik = turnIK;
                    auto position = turnPosition;
                    for (int frame = 0; frame < pauseFrames; ++frame) {
                        const int support = advance(ik, rig, position);
                        if (frame + 1 == fullStopFrames) {
                            assert(support == RobotSpiderIK::LegCount);
                        }
                    }
                    for (int frame = 0; frame < fps / 2; ++frame) {
                        position -= frameTravel;
                        advance(ik, rig, position);
                    }
                }
            }
        }
        std::cout << fps << "fps diagonal reversal error=" << maxError << " planted slide=" << maxSlide
                  << std::endl;
        assert(maxError < 0.15f && maxSlide < 0.15f);
    }
}

void CheckSlopedContacts(const RobotAnimationRig& original) {
    auto rig = original;
    RobotSpiderIK ik;
    assert(ik.Init(rig));
    const auto heightAt = [](float x, float z) { return -5 + 0.25f * x + 0.35f * z; };
    constexpr float dt = 1.0f / 60;
    for (int frame = 0; frame < 120; ++frame) {
        const float z = static_cast<float>((std::min)(frame, 90)) * 0.70f;
        const float ground = heightAt(0, z);
        const auto previous = ik.GetFeet();
        rig.Update(RobotMotion::Idle, dt);
        ik.Update(rig, dt, World(aiVector3D(0, ground, z)), ground, true, heightAt);
        for (size_t i = 0; i < ik.GetFeet().size(); ++i) {
            const auto& foot = ik.GetFeet()[i];
            const float clearance = foot.position.y - heightAt(foot.position.x, foot.position.z);
            assert(clearance >= -0.001f);
            if (!foot.swinging) {
                assert(std::abs(clearance) < 0.001f);
                assert((ik.GetSolvedContacts()[i] - foot.position).Length() < 0.15f);
                if (frame > 0 && !previous[i].swinging) {
                    assert((foot.position - previous[i].position).Length() < 0.001f);
                }
            }
        }
    }
}

void CheckSlopedJump(const RobotAnimationRig& original, bool walking, int fps, bool steep = false) {
    auto rig = original;
    RobotSpiderIK ik;
    assert(ik.Init(rig));
    const float dt = 1.0f / fps;
    const int root = rig.FindNode("Root_M");
    const int top = rig.FindNode("Top_M");
    const auto restRoot = original.GetMatrices()[root];
    const auto restUp = aiMatrix3x3(restRoot).Inverse() * aiVector3D(0, 1, 0);
    const auto bodyUp = [&](const aiMatrix4x4& world) {
        auto actual = aiMatrix3x3(world * rig.GetMatrices()[root]) * restUp;
        return actual.Normalize();
    };
    // FBXの基準姿勢を使い、胴体からの足先の変位をモデル軸・ワールド単位で比較する。
    const auto bodyFeet = [&](const aiMatrix4x4& world) {
        const auto toRest = restRoot * (world * rig.GetMatrices()[root]).Inverse();
        auto feet = ik.GetSolvedContacts();
        for (auto& foot : feet) {
            foot = (toRest * foot - restRoot * aiVector3D()) * 0.025f;
        }
        return feet;
    };
    const auto distanceXZ = [](aiVector3D a, aiVector3D b) { return std::hypot(a.x - b.x, a.z - b.z); };

    const aiVector3D takeoffSlope = steep ? aiVector3D(0.25f, 0, 0.35f) : aiVector3D(0.07f, 0, 0.04f);
    const aiVector3D landingSlope = steep ? aiVector3D(-0.2f, 0, -0.15f) : aiVector3D(-0.04f, 0, -0.03f);
    const auto normal = [](aiVector3D slope) { return aiVector3D(-slope.x, 1, -slope.z).Normalize(); };
    const auto takeoffHeight = [&](float x, float z) { return takeoffSlope.x * x + takeoffSlope.z * z; };
    aiVector3D position;
    auto takeoff = World(position, 0.4f);
    ik.Update(rig, dt, takeoff, 0, true);
    const auto stance = bodyFeet(takeoff);
    ik.Reset();
    ik.Update(rig, dt, takeoff, 0, false);
    ik.Update(rig, dt, takeoff, 0, true, takeoffHeight);
    assert((bodyUp(takeoff) - normal(takeoffSlope)).Length() < 0.001f);
    if (walking) {
        const auto raised = [&](const RobotSpiderIK::Foot& foot) {
            return foot.swinging && foot.position.y > takeoffHeight(foot.position.x, foot.position.z) + 0.4f;
        };
        for (int frame = 0; frame < fps; ++frame) {
            position.z += 60.0f * dt;
            position.y = takeoffHeight(position.x, position.z);
            takeoff = World(position, 0.4f);
            rig.Update(RobotMotion::Idle, dt);
            ik.Update(rig, dt, takeoff, position.y, true, takeoffHeight);
            if (std::any_of(ik.GetFeet().begin(), ik.GetFeet().end(), raised)) {
                break;
            }
        }
        assert(std::any_of(ik.GetFeet().begin(), ik.GetFeet().end(), raised));
    }
    const auto topAtTakeoff = aiMatrix4x4(rig.GetMatrices()[root]).Inverse() * rig.GetMatrices()[top];
    const float travel = walking ? 8.0f : 0.0f;
    const auto destination = position + aiVector3D(travel, 0, 0);
    const auto landingHeight = [&](float x, float z) {
        return destination.y + landingSlope.x * (x - destination.x) + landingSlope.z * (z - destination.z);
    };
    std::vector<aiVector3D> queries;
    const auto terrainBelow = [&](float x, float z) {
        queries.emplace_back(x, 0.0f, z);
        return landingHeight(x, z);
    };

    // 同じ重力で、静止からは2秒、歩行からは4/3秒の放物線をたどる。
    const int frames = walking ? fps * 4 / 3 : fps * 2;
    const float duration = frames * dt;
    const float slopeRise = 9.0f * std::hypot(takeoffSlope.x, takeoffSlope.z);
    auto apex = stance, gathered = stance;
    auto previous = bodyFeet(takeoff);
    auto previousUp = bodyUp(takeoff);
    for (int frame = 1; frame < frames; ++frame) {
        const float time = frame * dt;
        const float phase = static_cast<float>(frame) / frames;
        const float horizontalPhase = (std::min)(phase / 0.8f, 1.0f);
        const float travelRatio = horizontalPhase * horizontalPhase * (3 - 2 * horizontalPhase);
        const float height = 0.5f * 28.8f * time * (duration - time);
        const auto center = position + aiVector3D(travel * travelRatio, height, 0);
        const auto world = World(center, 0.4f + 0.8f * travelRatio);
        const float ground = landingHeight(center.x, center.z);
        const size_t queryCount = queries.size();
        rig.Update(RobotMotion::Idle, dt, 1, phase, 0.4f * phase);
        ik.Update(rig, dt, world, ground, false, terrainBelow);
        // この表示フレームの平均上下速度。IKの細分更新にも同じ直線補間が入る。
        const float fallSpeed = 28.8f * (time - 0.5f * (duration + dt));
        const float approachHeight = (std::max)(3.0f, fallSpeed * 0.22f) + slopeRise;
        if (frame <= frames / 2 || center.y - ground > approachHeight + 0.001f) {
            assert(queries.size() == queryCount);
            if (!walking) {
                assert((bodyUp(world) - normal(takeoffSlope)).Length() < 0.001f);
            }
        }
        const auto up = bodyUp(world);
        assert((up - previousUp).Length() < 10.0f * dt + 0.005f);
        previousUp = up;
        const auto feet = bodyFeet(world);
        for (size_t i = 0; i < feet.size(); ++i) {
            assert((feet[i] - previous[i]).Length() < 70.0f * dt + 0.02f);
            assert((ik.GetFeet()[i].position - ik.GetSolvedContacts()[i]).Length() < 0.15f);
            for (int segment = 0; segment < 3; ++segment) {
                const int a = LegJoints[i][segment], b = LegJoints[i][segment + 1];
                const auto length = [&](const auto& pose) {
                    return (pose[a] * aiVector3D() - pose[b] * aiVector3D()).Length();
                };
                assert(std::abs(length(original.GetMatrices()) - length(rig.GetMatrices())) < 0.02f);
            }
        }
        if (frame == frames / 2) {
            apex = feet;
            const auto topNow = aiMatrix4x4(rig.GetMatrices()[root]).Inverse() * rig.GetMatrices()[top];
            assert(!topNow.Equal(topAtTakeoff, 0.01f));
        }
        // 急坂では低い側の脚が先に地面へ向かうため、全脚の持ち上げは下降前半で確かめる。
        if (steep && frame == frames * 13 / 20) {
            for (size_t i = 0; i < feet.size(); ++i) {
                assert(feet[i].y > apex[i].y + 0.15f);
            }
        }
        if (frame == frames * 3 / 4) {
            gathered = feet;
        }
        previous = feet;
    }

    const auto beforeLanding = ik.GetSolvedContacts();
    const auto world = World(destination, 1.2f);
    rig.Update(RobotMotion::Idle, dt);
    ik.Update(rig, dt, world, destination.y, true, landingHeight);
    if (!walking) {
        assert((previousUp - normal(landingSlope)).Length() < 0.08f);
        assert((bodyUp(world) - normal(landingSlope)).Length() < 0.001f);
    }
    const auto landed = ik.GetSolvedContacts();
    for (size_t i = 0; i < landed.size(); ++i) {
        assert(distanceXZ(apex[i], {}) > distanceXZ(stance[i], {}) + 0.2f);
        assert(distanceXZ(gathered[i], {}) < distanceXZ(apex[i], {}) - 0.3f);
        assert(distanceXZ(gathered[i], {}) < distanceXZ(stance[i], {}) - 0.1f);
        if (!steep) {
            assert(gathered[i].y > apex[i].y + 0.15f);
            assert(previous[i].y < gathered[i].y - 0.1f);
        }
        assert(distanceXZ(previous[i], {}) < distanceXZ(stance[i], {}) - 0.1f);
        assert(std::any_of(queries.begin(), queries.end(),
                           [&](aiVector3D query) { return distanceXZ(query, beforeLanding[i]) < 0.4f; }));
        assert(distanceXZ(landed[i], beforeLanding[i]) < 12.0f * dt + 0.02f);
        const float finalClearance =
            beforeLanding[i].y - landingHeight(beforeLanding[i].x, beforeLanding[i].z);
        assert(finalClearance >= -0.01f && finalClearance < 60.0f * dt + 0.05f);
        assert(!ik.GetFeet()[i].swinging);
        assert(std::abs(landed[i].y - landingHeight(landed[i].x, landed[i].z)) < 0.01f);
    }
    for (int frame = 0; frame < fps / 3; ++frame) {
        rig.Update(RobotMotion::Idle, dt);
        ik.Update(rig, dt, world, destination.y, true, landingHeight);
        for (size_t i = 0; i < landed.size(); ++i) {
            assert(!ik.GetFeet()[i].swinging && (ik.GetSolvedContacts()[i] - landed[i]).Length() < 0.01f);
        }
    }

    for (int frame = 1; frame <= fps; ++frame) {
        auto center = destination + aiVector3D(0, 0, 30.0f * frame * dt);
        center.y = landingHeight(center.x, center.z);
        rig.Update(RobotMotion::Idle, dt);
        ik.Update(rig, dt, World(center, 1.2f), center.y, true, landingHeight);
        const auto& feet = ik.GetFeet();
        assert(std::count_if(feet.begin(), feet.end(), [](const auto& f) { return !f.swinging; }) >= 3);
    }
    for (const auto& foot : ik.GetFeet()) {
        assert(foot.stepNumber > 0);
    }
}

void CheckLiveSettings(const RobotAnimationRig& original) {
    auto rig = original;
    RobotSpiderIK ik;
    ik.m_settings.stanceWidth = 0.70f;
    ik.m_settings.bodyHeight = 1.8f;
    assert(ik.Init(rig));
    const auto world = World(aiVector3D(0, -5, 0));
    for (int frame = 0; frame < 30; ++frame) {
        rig.Update(RobotMotion::Idle, 1.0f / 60);
        ik.Update(rig, 1.0f / 60, world, -5, true);
    }
    const auto narrow = ik.GetFeet();
    const auto originalSettings = ik.m_settings;
    const auto clockBefore = rig.GetTime();

    // ゲーム更新を呼ばずに設定を変更する。一時停止中でも足が外側へ広がる。
    auto wideSettings = originalSettings;
    wideSettings.stanceWidth = 0.90f;
    wideSettings.bodyHeight = 0.4f;
    ik.ApplySettings(rig, wideSettings);
    const auto wide = ik.GetFeet();
    for (size_t i = 0; i < wide.size(); ++i) {
        const auto oldRadius = aiVector3D(narrow[i].position.x, 0, narrow[i].position.z).Length();
        const auto newRadius = aiVector3D(wide[i].position.x, 0, wide[i].position.z).Length();
        assert(newRadius > oldRadius + 1.0f);
        assert(wide[i].stepNumber == narrow[i].stepNumber);
        assert(wide[i].progress == narrow[i].progress);
        assert((ik.GetSolvedContacts()[i] - wide[i].position).Length() < 0.15f);
    }
    assert(rig.GetTime() == clockBefore);
    for (int repeat = 0; repeat < 50; ++repeat) {
        ik.ApplySettings(rig, wideSettings);
    }
    for (size_t i = 0; i < wide.size(); ++i) {
        assert((ik.GetFeet()[i].position - wide[i].position).Length() < 0.0001f);
    }

    // 往復操作で拡大が累積しない。歩幅だけを変更しても、停止中の接地点は動かない。
    ik.ApplySettings(rig, originalSettings);
    auto strideSettings = originalSettings;
    strideSettings.strideLength = RobotSpiderIK::TuningLimits::MaxStrideLength;
    ik.ApplySettings(rig, strideSettings);
    assert(ik.m_settings.strideLength == strideSettings.strideLength);
    for (size_t i = 0; i < narrow.size(); ++i) {
        assert((ik.GetFeet()[i].position - narrow[i].position).Length() < 0.001f);
    }

    // 歩行途中でも足をリセットせず、現在の一歩を継続できる。
    aiVector3D position(0, -5, 0);
    ik.ApplySettings(rig, originalSettings);
    for (int frame = 0; frame < 11; ++frame) {
        position.z += 1.0f;
        rig.Update(RobotMotion::Idle, 1.0f / 60);
        ik.Update(rig, 1.0f / 60, World(position), -5, true);
    }
    const auto moving = ik.GetFeet();
    ik.ApplySettings(rig, wideSettings);
    for (size_t i = 0; i < moving.size(); ++i) {
        const auto& foot = ik.GetFeet()[i];
        assert(foot.swinging == moving[i].swinging);
        assert(foot.progress == moving[i].progress);
        assert(foot.stepNumber == moving[i].stepNumber);
        const auto shift = foot.position - moving[i].position;
        assert((foot.target - moving[i].target - shift).Length() < 0.001f);
    }

    // 手入力でも範囲外やNaNの設定が骨行列へ混ざらない。
    auto invalid = wideSettings;
    invalid.strideLength = -100.0f;
    invalid.stanceWidth = std::numeric_limits<float>::quiet_NaN();
    invalid.bodyHeight = 100.0f;
    invalid.stepHeight = -100.0f;
    ik.ApplySettings(rig, invalid);
    assert(ik.m_settings.strideLength == RobotSpiderIK::TuningLimits::MinStrideLength);
    assert(ik.m_settings.stanceWidth == RobotSpiderIK::Settings{}.stanceWidth);
    assert(ik.m_settings.bodyHeight == 2.5f && ik.m_settings.stepHeight == 0.2f);
    invalid.strideLength = 100.0f;
    ik.ApplySettings(rig, invalid);
    assert(ik.m_settings.strideLength == RobotSpiderIK::TuningLimits::MaxStrideLength);
    for (const auto& matrix : rig.GetMatrices()) {
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                assert(std::isfinite(matrix[row][col]));
            }
        }
    }
    std::cout << "Live stance/height preview, no drift, paused clock and setting bounds passed.\n";
}

void CheckBodyAndFootControls(const RobotAnimationRig& original) {
    auto rig = original;
    RobotSpiderIK ik;
    assert(ik.Init(rig));
    constexpr float dt = 1.0f / 60;
    constexpr float yaw = 0.67f;
    const auto heightAt = [](float x, float z) { return 0.08f * x - 0.05f * z; };
    const float ground = heightAt(12, -4);
    const auto world = World(aiVector3D(12, ground, -4), yaw);
    const auto heading = aiQuaternion(aiVector3D(0, 1, 0), yaw).GetMatrix();
    for (int frame = 0; frame < 120; ++frame) {
        rig.Update(RobotMotion::Idle, dt);
        ik.Update(rig, dt, world, ground, true, heightAt);
    }
    const auto clockBefore = rig.GetTime();
    const auto baseRoot = *rig.FindNodeTransform("Root_M");
    const auto baseCenter = world * (baseRoot * aiVector3D());
    const aiMatrix3x3 restOrientation(*original.FindNodeTransform("Root_M"));
    const auto slopeOrientation = aiMatrix3x3(baseRoot) * aiMatrix3x3(restOrientation).Inverse();
    const auto baseFeet = ik.GetFeet();
    std::array<aiVector3D, RobotSpiderIK::LegCount> targets;
    for (size_t i = 0; i < targets.size(); ++i) {
        targets[i] = baseFeet[i].position;
    }
    const auto checkFeet = [&] {
        for (size_t i = 0; i < targets.size(); ++i) {
            assert(!ik.GetFeet()[i].swinging && ik.GetFeet()[i].stepNumber == baseFeet[i].stepNumber);
            assert((ik.GetFeet()[i].position - targets[i]).Length() < 0.001f);
            assert((ik.GetSolvedContacts()[i] - targets[i]).Length() < 0.005f);
        }
    };

    auto settings = ik.m_settings;
    settings.bodyRight = 0.45f;
    settings.bodyForward = -0.3f;
    settings.bodyHeight += 0.15f;
    const auto shift = heading * aiVector3D(-settings.bodyRight, 0.15f, -settings.bodyForward);
    // 各軸を個別に回し、旋回済みの機体でも胴体だけが意図した軸・向きへ回ることを確認する。
    for (int axis = 0; axis < 3; ++axis) {
        settings.bodyRotation = {};
        settings.bodyRotation[axis] = 0.08f;
        ik.ApplySettings(rig, settings, heightAt);
        const auto root = *rig.FindNodeTransform("Root_M");
        assert((world * (root * aiVector3D()) - baseCenter - shift).Length() < 0.001f);
        aiVector3D localAxis;
        localAxis[axis] = 1;
        const auto rotation = aiQuaternion(localAxis, settings.bodyRotation[axis]).GetMatrix();
        assert(aiMatrix3x3(root).Equal(slopeOrientation * rotation * restOrientation, 0.001f));
        checkFeet();
    }
    assert(rig.GetTime() == clockBefore);

    // 右中脚だけを横・上・前へ動かす。高さの基準は移動先の地表。
    constexpr size_t selected = 1;
    settings.footOffsets[selected] = aiVector3D(0.7f, 0.8f, 0.4f);
    const auto offset = settings.footOffsets[selected];
    targets[selected] += heading * aiVector3D(-offset.x, 0, -offset.z);
    targets[selected].y = heightAt(targets[selected].x, targets[selected].z) + offset.y;
    const auto posedRoot = *rig.FindNodeTransform("Root_M");
    ik.ApplySettings(rig, settings, heightAt);
    checkFeet();
    assert(rig.GetTime() == clockBefore);
    assert(rig.FindNodeTransform("Root_M")->Equal(posedRoot, 0.001f));
    for (int frame = 0; frame < 30; ++frame) {
        rig.Update(RobotMotion::Idle, dt);
        ik.Update(rig, dt, world, ground, true, heightAt);
        checkFeet();
        assert(rig.FindNodeTransform("Root_M")->Equal(posedRoot, 0.001f));
    }

    ik.ApplySettings(rig, RobotSpiderIK::Settings{}, heightAt);
    targets[selected] = baseFeet[selected].position;
    checkFeet();
    assert(rig.FindNodeTransform("Root_M")->Equal(baseRoot, 0.001f));
}

void CheckExpandedStride(const RobotAnimationRig& original) {
    constexpr float MovementSpeed = 60.0f; // 通常のプレイヤー速度で、一歩の時間上限も検証する。
    constexpr float PreviousMaximumStride = 24.0f;
    constexpr int SimulationSeconds = 4;

    for (int fps : {30, 60, 144}) {
        float previousAverageStride = 0.0f;
        for (float requestedStride : {PreviousMaximumStride, RobotSpiderIK::TuningLimits::MaxStrideLength}) {
            auto rig = original;
            RobotSpiderIK ik;
            assert(ik.Init(rig));
            auto tuning = ik.m_settings;
            tuning.strideLength = requestedStride;
            ik.ApplySettings(rig, tuning);

            const float dt = 1.0f / fps;
            aiVector3D position(0, -5, 0);
            ik.Update(rig, dt, World(position), -5, true);
            float totalStride = 0.0f;
            unsigned stepCount = 0;

            for (int frame = 0; frame < fps * SimulationSeconds; ++frame) {
                const auto previousFeet = ik.GetFeet();
                position.z += MovementSpeed * dt;
                rig.Update(RobotMotion::Idle, dt);
                ik.Update(rig, dt, World(position), -5, true);

                int supportCount = 0;
                for (size_t index = 0; index < previousFeet.size(); ++index) {
                    const auto& foot = ik.GetFeet()[index];
                    supportCount += !foot.swinging;
                    if (frame >= fps && foot.stepNumber != previousFeet[index].stepNumber) {
                        totalStride += (foot.target - foot.start).Length();
                        ++stepCount;
                    }
                }
                assert(supportCount >= 3); // 大股でも両方の3本組を同時に上げない。

                for (const auto& matrix : rig.GetMatrices()) {
                    for (int row = 0; row < 4; ++row) {
                        for (int column = 0; column < 4; ++column) {
                            assert(std::isfinite(matrix[row][column]));
                        }
                    }
                }
            }

            assert(stepCount > 0);
            const float averageStride = totalStride / stepCount;
            assert(std::abs(averageStride - requestedStride) < 1.0f);
            if (requestedStride == PreviousMaximumStride) {
                previousAverageStride = averageStride;
            } else {
                // 表示上の上限だけでなく、時間制限に阻まれず実際の予定歩幅も広がる。
                assert(averageStride > previousAverageStride + 6.0f);
                std::cout << fps << "fps expanded stride=" << averageStride << '\n';
            }

            const auto stopped = ik.GetFeet();
            for (int frame = 0; frame < fps; ++frame) {
                rig.Update(RobotMotion::Idle, dt);
                ik.Update(rig, dt, World(position), -5, true);
            }
            for (size_t index = 0; index < stopped.size(); ++index) {
                const auto& foot = ik.GetFeet()[index];
                assert(!foot.swinging);
                assert(foot.stepNumber == stopped[index].stepNumber);
                assert(std::abs(foot.position.x - stopped[index].position.x) < 0.001f);
                assert(std::abs(foot.position.z - stopped[index].position.z) < 0.001f);
            }
        }
    }
}

int main() {
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    RobotAnimationRig original;
    std::string error;
    assert(original.Load("DX22_01_plane/assets/model/Player", error));
    original.SetLowerBodyScale(1.40f);
    CheckStrideAndStopping(original);
    CheckDiagonalReversals(original);
    CheckLiveSettings(original);
    CheckBodyAndFootControls(original);
    CheckSlopedContacts(original);
    for (int fps : {30, 60, 144}) {
        CheckSlopedJump(original, false, fps);
        CheckSlopedJump(original, true, fps);
        CheckSlopedJump(original, false, fps, true);
        CheckSlopedJump(original, true, fps, true);
    }
    CheckExpandedStride(original);
    const auto rest = original.GetMatrices();
    for (int fps : {30, 60, 144}) {
        auto rig = original;
        RobotSpiderIK ik;
        assert(ik.Init(rig));
        const float dt = 1.0f / fps;
        aiVector3D position(0, -5, 0);
        float maxError = 0, maxSlide = 0, maxTilt = 0, maxLag = 0;
        int lifted = 0;
        for (int frame = 0; frame < fps * 10; ++frame) {
            const float time = frame * dt;
            aiVector3D velocity;
            if (time >= 1 && time < 3) {
                velocity = aiVector3D(0, 0, 60);
            } else if (time >= 3 && time < 4) {
                velocity = aiVector3D(60, 0, 0);
            } else if (time >= 4 && time < 5) {
                velocity = aiVector3D(-42, 0, -42);
            } else if (time >= 5 && time < 6) {
                velocity = aiVector3D(0, 0, -96);
            }
            position += velocity * dt;
            const float yaw = time > 7 && time < 8 ? (time - 7) : (time >= 8 ? 1.0f : 0.0f);
            const auto previousFeet = ik.GetFeet();
            const auto previousContacts = ik.GetSolvedContacts();
            rig.Update(RobotMotion::Idle, dt, 1, 0.6f, 0.25f);
            ik.Update(rig, dt, World(position, yaw), -5, true);
            int support = 0;
            for (size_t i = 0; i < 6; ++i) {
                const auto& foot = ik.GetFeet()[i];
                support += !foot.swinging;
                if (foot.swinging && foot.position.y > -4.9f) {
                    ++lifted;
                }
                const float contactError = (ik.GetSolvedContacts()[i] - foot.position).Length();
                maxError = (std::max)(maxError, contactError);
                if (frame > 0 && !foot.swinging && !previousFeet[i].swinging &&
                    foot.stepNumber == previousFeet[i].stepNumber) {
                    maxSlide =
                        (std::max)(maxSlide, (ik.GetSolvedContacts()[i] - previousContacts[i]).Length());
                }
                for (int j = 0; j < 3; ++j) {
                    const int a = LegJoints[i][j], b = LegJoints[i][j + 1];
                    const float before = (rest[a] * aiVector3D() - rest[b] * aiVector3D()).Length();
                    const auto& pose = rig.GetMatrices();
                    const float after = (pose[a] * aiVector3D() - pose[b] * aiVector3D()).Length();
                    assert(std::abs(before - after) < 0.02f); // Never stretch rigid segments.
                }
            }
            assert(support >= 3);
            maxTilt = (std::max)(maxTilt, ik.GetBodyTilt().Length());
            maxLag = (std::max)(maxLag, ik.GetBodyOffset().Length());
            for (const auto& m : rig.GetMatrices()) {
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        assert(std::isfinite(m[r][c]));
                    }
                }
            }
        }
        std::cout << fps << "fps contact error=" << maxError << " planted slide=" << maxSlide
                  << " tilt=" << maxTilt << " lag=" << maxLag << std::endl;
        assert(maxError < 0.15f && maxSlide < 0.15f);
        assert(lifted > fps && maxTilt > 0.02f && maxLag > 0.2f);
        assert(ik.GetBodyTilt().Length() < 0.002f);
        const auto frozen = ik.GetSolvedContacts();
        ik.Update(rig, 0, World(position + aiVector3D(5, 0, 0)), -5, true);
        for (size_t i = 0; i < 6; ++i) {
            assert((frozen[i] - ik.GetSolvedContacts()[i]).Length() == 0);
        }
        // Jump/release anchors, land, then teleport without carrying old planted targets.
        for (int f = 0; f < fps; ++f) {
            rig.Update(RobotMotion::Idle, dt);
            ik.Update(rig, dt, World(position + aiVector3D(0, 10, 0)), -5, false);
        }
        ik.Update(rig, dt, World(position), -5, true);
        for (const auto& foot : ik.GetFeet()) {
            assert(std::abs(foot.position.y + 5) < 0.001f);
        }
        ik.Update(rig, dt, World(position + aiVector3D(500, 0, 0)), -5, true);
        for (size_t i = 0; i < 6; ++i) {
            assert((ik.GetSolvedContacts()[i] - ik.GetFeet()[i].position).Length() < 0.15f);
        }

        // Aiming changes only the upper branch, even while the body is sprung and feet are planted.
        auto aimedRig = rig;
        auto aimedIK = ik;
        const auto lastWorld = World(position + aiVector3D(500, 0, 0));
        rig.Update(RobotMotion::Idle, dt, 1, 0, 0);
        aimedRig.Update(RobotMotion::Idle, dt, 1, 1.0f, 0.4f);
        ik.Update(rig, dt, lastWorld, -5, true);
        aimedIK.Update(aimedRig, dt, lastWorld, -5, true);
        for (const auto& leg : LegJoints) {
            for (int joint : leg) {
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        assert(std::abs(rig.GetMatrices()[joint][r][c] -
                                        aimedRig.GetMatrices()[joint][r][c]) < 0.0001f);
                    }
                }
            }
        }

        RobotDebris debris;
        debris.Start(aimedRig, lastWorld, -5);
        assert(debris.IsActive());
        // Starting the death effect must preserve every vertex of the current IK pose.
        double before[3]{}, after[3]{};
        size_t count = 0;
        for (const auto& part : aimedRig.GetParts()) {
            for (const auto& vertex : part.vertices) {
                const auto p = lastWorld * aimedRig.GetMatrices()[vertex.node] * vertex.position;
                for (int axis = 0; axis < 3; ++axis) {
                    before[axis] += p[axis];
                }
                ++count;
            }
        }
        for (const auto& vertex : debris.GetVertices()) {
            const auto p = debris.GetMatrices()[vertex.node] * vertex.position;
            for (int axis = 0; axis < 3; ++axis) {
                after[axis] += p[axis];
            }
        }
        assert(count == debris.GetVertices().size());
        for (int axis = 0; axis < 3; ++axis) {
            assert(std::abs(before[axis] - after[axis]) / count < 0.001);
        }
    }
    std::cout << "Six-foot IK, support, foot locking, weight, pause, landing and teleport checks passed.\n";
}
