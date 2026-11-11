/// @file RobotSpiderIKTests.cpp
/// @brief 動作と境界条件を検証する RobotSpiderIKTests の自動テスト。

#include "../DX22_01_plane/RobotSpiderIK.h"
#include "../DX22_01_plane/RobotDebris.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

aiMatrix4x4 World(aiVector3D position, float yaw = 0) {
    return aiMatrix4x4(aiVector3D(0.025f), aiQuaternion(aiVector3D(0, 1, 0), yaw), position);
}

void CheckStrideAndStopping(const RobotAnimationRig& original) {
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
    CheckLiveSettings(original);
    CheckExpandedStride(original);
    const int joints[][4] = {{3, 4, 5, 6},     {9, 10, 11, 12},  {15, 16, 17, 18},
                             {27, 28, 29, 30}, {33, 34, 35, 36}, {39, 40, 41, 42}};
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
                    const int a = joints[i][j], b = joints[i][j + 1];
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
        for (const auto& leg : joints) {
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
