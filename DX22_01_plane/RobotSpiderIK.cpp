/// @file RobotSpiderIK.cpp
/// @brief 6本脚の接地点を保ちながら歩行と胴体の揺れを作るIK。

#include "RobotSpiderIK.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
    // 素材の構造。配列の偶数番号と奇数番号が、それぞれ交互に動かす3本組になる。
    constexpr int SideCount = 2;
    constexpr int LegsPerSide = 3;
    constexpr int TripodCount = 2;
    constexpr const char* LegPrefixes[] = {"BackLeg", "MiddleLeg", "FrontLeg"};

    // 計算精度と細分更新。低FPSでも一歩の開始・着地とバネの動きを安定させる。
    constexpr float Pi = 3.14159265359f;
    constexpr float MaxIntegrationStepSeconds = 1.0f / 240.0f;
    constexpr float MinSpringFrequencyHz = 0.1f;
    constexpr float RotationVectorSquaredEpsilon = 1e-8f;
    constexpr float IKVectorSquaredEpsilon = 1e-6f;
    constexpr float RemainingPhaseEpsilon = 0.0001f;

    // 足裏の検出とIKの可動範囲。ここだけはFBXのモデル座標での距離。
    constexpr float SoleVertexHeightBand = 0.5f; // 最低点からこの高さまでの頂点を足裏とみなす。
    constexpr float JointReachMargin = 0.01f; // 完全な伸び切り・折り畳みを避ける余裕。

    // 接地点を捨てて置き直す条件。ワープや長時間停止後に脚が引き伸ばされるのを防ぐ。
    constexpr float TeleportResetDistance = 35.0f;
    constexpr float LongFrameResetSeconds = 0.25f;

    // 足運びのタイミングと方向転換の調整値。
    constexpr float MovingSpeedThreshold = 0.1f; // これ以下の移動速度は停止扱い。
    constexpr float MinStrideCalculationSpeed = 1.0f; // 歩幅から時間を求める際のゼロ除算を防ぐ。
    constexpr float DirectionChangeCosine = 0.85f; // 前回の進行方向との内積がこれ未満なら切り返し。
    constexpr float SpeedIncreaseRatio = 1.2f; // 急な加速も短い踏み替えで追従する。
    constexpr int TurnRecoveryTripods = 2; // 切り返し後、両方の3本組を一度ずつ早めに着地。
    constexpr float TurnLandingSeconds = 0.06f; // 切り返し時の残りの飛行時間の上限。
    constexpr float StopLandingSeconds = 0.12f; // 入力を離してから真下へ足を下ろす時間。
    constexpr float ShortStepDurationRatio = 0.5f; // 歩き始め・切り返し時は通常の半分の時間。
    constexpr float LandingLeadRatio = 0.5f; // 次の支持期間の中央を見越して着地先を前へ出す。
    constexpr float AirborneFootLift = 1.2f; // 本体が空中にいるときの足の引き上げ量。
    constexpr float TurningRotationDot = 0.9999999f; // 停止中の下半身の回転を検出する内積の境界。

    // 胴体の重さを表現する調整値。移動速度・加速度から遅れと傾きを作る。
    constexpr float VelocitySmoothingRate = 10.0f; // 移動速度をならす速さ（毎秒）。
    constexpr float MaxBodyAcceleration = 350.0f; // 急な入力による傾きの跳ねを抑える加速度の上限。
    constexpr float LandingBodyImpulse = 4.0f; // 本体が着地したときの下向き速度。
    constexpr float FootfallBodyImpulse = 0.35f; // 1本の足が着地したときの下向き速度。
    constexpr float BodyLagSeconds = 0.012f; // 胴体を移動方向の後ろへ遅らせる時間相当量。
    constexpr float MaxBodyLagDistance = 1.0f; // 胴体の水平な遅れの上限。
    constexpr float RestBodyOffsetY = -0.15f; // 接地中の基本の沈み込み。
    constexpr float MovingBodySink = 0.20f; // 歩行時に追加する沈み込み。
    constexpr float BodySinkReferenceSpeed = 60.0f; // 沈み込みが最大になる移動速度。
    constexpr float AirborneBodyOffsetY = 0.25f; // 空中での胴体の持ち上げ量。
    constexpr float MinBodyOffsetY = -1.2f; // バネで沈み込める下限。
    constexpr float MaxBodyOffsetY = 0.8f; // バネで浮き上がれる上限。
    constexpr float VelocityTiltWeight = 0.0007f; // 移動速度を胴体の傾きへ変換する係数。
    constexpr float AccelerationTiltWeight = 0.00012f; // 加速度を胴体の傾きへ変換する係数。
    constexpr float MaxBodyTiltRadians = 0.10f; // 前後・左右それぞれの傾きの上限。

    // 行列が表す原点の位置を取り出す。
    aiVector3D Position(const aiMatrix4x4& matrix) {
        return matrix * aiVector3D();
    }

    // 足運びの判定に高さを混ぜないよう、水平成分だけを残す。
    aiVector3D Horizontal(aiVector3D value) {
        value.y = 0.0f;
        return value;
    }

    aiVector3D Limited(aiVector3D value, float maximum) {
        const float length = value.Length();
        return length > maximum ? value * (maximum / length) : value;
    }

    // 原点中心の回転行列を、指定した関節pivotを中心に回る行列へ直す。
    aiMatrix4x4 Around(aiMatrix4x4 rotation, const aiVector3D& pivot) {
        const auto offset = pivot - rotation * pivot;
        rotation.a4 = offset.x;
        rotation.b4 = offset.y;
        rotation.c4 = offset.z;
        return rotation;
    }

    // 骨の向きfromをtoへ合わせる。骨の長さは変えず、関節を中心に回転する。
    aiMatrix4x4 Align(aiVector3D from, aiVector3D to, const aiVector3D& pivot) {
        if (from.SquareLength() < RotationVectorSquaredEpsilon ||
            to.SquareLength() < RotationVectorSquaredEpsilon) {
            return aiMatrix4x4();
        }

        from.Normalize();
        to.Normalize();
        const float cosine = std::clamp(from * to, -1.0f, 1.0f);
        auto axis = from ^ to; // 外積が、2つの方向をつなぐ回転軸になる。

        if (axis.SquareLength() < RotationVectorSquaredEpsilon) {
            if (cosine > 0.0f) {
                return aiMatrix4x4(); // 既に同じ方向を向いている。
            }

            // 正反対の場合は外積が0になるため、直交する軸を選び直す。
            axis = from ^ aiVector3D(0, 1, 0);
            if (axis.SquareLength() < RotationVectorSquaredEpsilon) {
                axis = from ^ aiVector3D(1, 0, 0);
            }
        }

        axis.Normalize();
        return Around(aiMatrix4x4(aiQuaternion(axis, std::acos(cosine)).GetMatrix()), pivot);
    }

    float ValidSetting(float value, float fallback, float minimum, float maximum) {
        // 数値でない入力は初期値へ戻し、通常の入力は調整可能範囲に収める。
        return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
    }
} // namespace

void RobotSpiderIK::Spring::Update(aiVector3D target, float dt, float frequency, float damping) {
    const float omega = 2.0f * Pi * (std::max)(MinSpringFrequencyHz, frequency); // 角振動数。
    const int steps = (std::max)(1, static_cast<int>(std::ceil(dt / MaxIntegrationStepSeconds)));
    const float stepSeconds = dt / steps; // 低FPSでも、短い時間に分けて積分する。

    for (int step = 0; step < steps; ++step) {
        // 目標へ戻す力から、揺れを減らす抵抗を引く（減衰バネの運動方程式）。
        const auto restoringForce = (target - value) * (omega * omega);
        const auto dampingForce = velocity * (2.0f * damping * omega);
        velocity += (restoringForce - dampingForce) * stepSeconds;
        value += velocity * stepSeconds;
    }
}

bool RobotSpiderIK::Init(const RobotAnimationRig& rig) {
    // 再初期化しても、先に指定した歩幅などの調整値は失わない。
    const auto tuning = m_settings;
    *this = RobotSpiderIK{};
    m_settings = tuning;
    m_rest = rig.m_globals;

    for (int side = 0; side < SideCount; ++side) {
        for (int row = 0; row < LegsPerSide; ++row) {
            auto& leg = m_legs[side * LegsPerSide + row];
            for (int joint = 0; joint < JointCount; ++joint) {
                const auto name =
                    std::string(LegPrefixes[row]) + std::to_string(joint + 1) + (side == 0 ? "_R" : "_L");
                leg.joints[joint] = rig.FindNode(name);
                if (leg.joints[joint] < 0) {
                    return false;
                }
            }

            // FBXのEndマーカーは見える足裏より下にあるため、足首に属する頂点から足裏を探す。
            const int ankle = leg.joints[Ankle];
            float bottom = std::numeric_limits<float>::max(); // 足首メッシュの最も低い高さ。
            for (const auto& part : rig.m_parts) {
                for (const auto& vertex : part.vertices) {
                    if (vertex.node == ankle) {
                        bottom = (std::min)(bottom, (m_rest[ankle] * vertex.position).y);
                    }
                }
            }

            if (bottom == std::numeric_limits<float>::max()) {
                return false;
            }

            aiVector3D contact; // 足裏付近の頂点の平均を、足先の接地点にする。
            int contactVertexCount = 0;
            for (const auto& part : rig.m_parts) {
                for (const auto& vertex : part.vertices) {
                    if (vertex.node != ankle) {
                        continue;
                    }

                    const auto position = m_rest[ankle] * vertex.position;
                    if (position.y <= bottom + SoleVertexHeightBand) {
                        contact += position;
                        ++contactVertexCount;
                    }
                }
            }

            contact /= static_cast<float>(contactVertexCount);
            contact.y = bottom;
            leg.restContact = contact;
            leg.home = GetHome(leg);
            leg.ankleOffset = Position(m_rest[ankle]) - contact;

            auto inverseAnkle = m_rest[ankle];
            inverseAnkle.Inverse();
            leg.localContact = inverseAnkle * contact;
        }
    }

    m_initialized = true;
    return true;
}

aiVector3D RobotSpiderIK::GetHome(const Leg& leg) const {
    // 毎回FBXの元の接地点から計算する。スライダーを往復しても倍率が累積しない。
    const auto base = Position(m_rest[leg.joints[Coxa]]);
    auto home = leg.restContact;
    home.x = base.x + (home.x - base.x) * m_settings.stanceWidth;
    home.z = base.z + (home.z - base.z) * m_settings.stanceWidth;
    return home;
}

void RobotSpiderIK::ApplySettings(RobotAnimationRig& rig, const Settings& requested) {
    m_settings = requested;
    const Settings defaults; // NaNなどの異常な入力に対して使う初期値。
    m_settings.strideLength = ValidSetting(m_settings.strideLength, defaults.strideLength,
                                           TuningLimits::MinStrideLength, TuningLimits::MaxStrideLength);
    m_settings.stanceWidth = ValidSetting(m_settings.stanceWidth, defaults.stanceWidth,
                                          TuningLimits::MinStanceWidth, TuningLimits::MaxStanceWidth);
    m_settings.bodyHeight = ValidSetting(m_settings.bodyHeight, defaults.bodyHeight,
                                         TuningLimits::MinBodyHeight, TuningLimits::MaxBodyHeight);
    m_settings.stepHeight = ValidSetting(m_settings.stepHeight, defaults.stepHeight,
                                         TuningLimits::MinStepHeight, TuningLimits::MaxStepHeight);

    if (!m_initialized) {
        return;
    }

    for (size_t index = 0; index < m_legs.size(); ++index) {
        auto& leg = m_legs[index];
        const auto previousHome = leg.home;
        leg.home = GetHome(leg);
        if (!m_tracking) {
            continue;
        }

        // 開き具合を変更した瞬間だけ接地点を外側／内側へ動かす。
        // 歩行中の前後位置と進行率は保つ。停止後にIdleの足位置へ戻さない。
        const auto shift = Horizontal(aiMatrix3x3(m_previousWorld) * (leg.home - previousHome));
        m_feet[index].position += shift;
        m_feet[index].start += shift;
        m_feet[index].target += shift;
    }

    if (m_tracking) {
        Solve(rig, m_previousWorld); // 時間を進めないため、一時停止中でも配置を確認できる。
    }
}

float RobotSpiderIK::GetContactError() const {
    if (!m_tracking) {
        return 0.0f;
    }

    float maximum = 0.0f;
    for (size_t index = 0; index < m_feet.size(); ++index) {
        const float error = (m_solvedContacts[index] - m_feet[index].position).Length();
        maximum = (std::max)(maximum, error);
    }
    return maximum;
}

void RobotSpiderIK::Reset() {
    m_tracking = false;
    m_body = {};
    m_tilt = {};
    m_filteredVelocity = {};
    m_nextTripod = 0;
    m_wasMoving = false;
    m_previousVelocity = {};
    m_turnSteps = 0;
}

void RobotSpiderIK::ResetContacts(const aiMatrix4x4& world, float groundY, bool grounded) {
    // 初回・ワープ・着地では、古い接地点を持ち越さず現在の胴体の周囲へ置き直す。
    for (size_t index = 0; index < m_feet.size(); ++index) {
        auto& foot = m_feet[index];
        foot = {};
        foot.position = world * m_legs[index].home;
        if (grounded) {
            foot.position.y = groundY;
        }
    }
}

void RobotSpiderIK::Update(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, float groundY,
                           bool grounded) {
    if (!m_initialized || dt <= 0.0f) {
        return;
    }

    const bool teleported = (Position(world) - m_previousPosition).Length() > TeleportResetDistance;
    if (!m_tracking || teleported || dt > LongFrameResetSeconds) {
        Step(rig, dt, world, groundY, grounded);
        m_previousWorld = world;
        return;
    }

    // 大きな1フレームを短い時間に分ける。30FPSでも着地のタイミングが飛ばないようにする。
    aiVector3D oldScale, newScale; // 前フレームと現在のモデル倍率。
    aiVector3D oldPosition, newPosition; // 前フレームと現在の本体位置。
    aiQuaternion oldRotation, newRotation; // 回転はクォータニオンで補間する。
    m_previousWorld.Decompose(oldScale, oldRotation, oldPosition);
    world.Decompose(newScale, newRotation, newPosition);

    const int steps = (std::max)(1, static_cast<int>(std::ceil(dt / MaxIntegrationStepSeconds)));
    for (int step = 1; step <= steps; ++step) {
        const float alpha = static_cast<float>(step) / steps; // 現在の細分更新までの進行率。
        aiQuaternion rotation;
        aiQuaternion::Interpolate(rotation, oldRotation, newRotation, alpha);
        const aiMatrix4x4 interpolated(oldScale + (newScale - oldScale) * alpha, rotation.Normalize(),
                                       oldPosition + (newPosition - oldPosition) * alpha);
        Step(rig, dt / steps, interpolated, groundY, grounded);
    }

    m_previousWorld = world;
}

void RobotSpiderIK::Step(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, float groundY,
                         bool grounded) {
    const auto position = Position(world);
    const auto displacement = position - m_previousPosition;
    const bool reset =
        !m_tracking || displacement.Length() > TeleportResetDistance || dt > LongFrameResetSeconds;

    if (reset) {
        Reset();
        m_tracking = true;
        m_previousPosition = position;
        ResetContacts(world, groundY, grounded);
        m_grounded = grounded;
    }

    // 足運びには実際の速度を使い、胴体の揺れには急な変化をならした速度を使う。
    const auto velocity = reset ? aiVector3D() : Horizontal(displacement) / dt;
    const float speed = velocity.Length();
    const auto previousFilteredVelocity = m_filteredVelocity;
    m_filteredVelocity += (velocity - m_filteredVelocity) * (1.0f - std::exp(-VelocitySmoothingRate * dt));
    const auto acceleration =
        Limited((m_filteredVelocity - previousFilteredVelocity) / dt, MaxBodyAcceleration);
    m_previousPosition = position;

    if (grounded && !m_grounded) {
        ResetContacts(world, groundY, grounded);
        m_body.velocity.y -= LandingBodyImpulse;
    }
    m_grounded = grounded;

    const bool moving = grounded && speed > MovingSpeedThreshold;
    // 2つの3本組で一周期になるので、片方を動かす時間は「歩幅 / 速度 / 2」。
    const float stepSeconds =
        std::clamp(m_settings.strideLength / (TripodCount * (std::max)(speed, MinStrideCalculationSpeed)),
                   m_settings.minStepSeconds, m_settings.maxStepSeconds);

    const float previousSpeed = m_previousVelocity.Length();
    const bool directionChanged =
        moving && m_wasMoving && previousSpeed > MovingSpeedThreshold &&
        (velocity * m_previousVelocity < speed * previousSpeed * DirectionChangeCosine ||
         speed > previousSpeed * SpeedIncreaseRatio);

    if (directionChanged) {
        // 大股の途中で切り返すと、地面に残した脚が伸び切りやすい。
        // このときだけ早めに着地して次の3本へ渡し、直進中は長い周期を保つ。
        m_turnSteps = TurnRecoveryTripods;
        for (auto& foot : m_feet) {
            if (!foot.swinging) {
                continue;
            }

            const float phaseLeft = 1.0f - foot.progress;
            if (phaseLeft > RemainingPhaseEpsilon) {
                foot.duration = (std::min)(foot.duration, TurnLandingSeconds / phaseLeft);
            }
        }
    }

    if (m_wasMoving && !moving && grounded) {
        // 入力を離したら、浮いている足だけ現在の真下へ下ろす。
        // 先読みした遠い着地点や、Idleの基準位置へ向かって足を動かさない。
        for (auto& foot : m_feet) {
            if (!foot.swinging) {
                continue;
            }

            foot.start = foot.position;
            foot.target = foot.position;
            foot.target.y = groundY;
            foot.progress = 0.0f;
            foot.duration = StopLandingSeconds;
            foot.liftHeight = 0.0f;
            foot.velocity = {};
            foot.arcOffset = {};
        }
    }

    // 先に今の一歩を進める。全て着地できた場合だけ、次の3本を持ち上げられる。
    const bool swinging = UpdateSwingingFeet(dt, world, groundY, grounded, moving, velocity, stepSeconds);

    if (!grounded) {
        for (size_t index = 0; index < m_feet.size(); ++index) {
            auto& foot = m_feet[index];
            foot.swinging = false;
            foot.position = world * m_legs[index].home + aiVector3D(0, AirborneFootLift, 0);
        }
    } else if (!swinging) {
        // 停止中は接地点を保持する。下半身そのものを回した場合は踏み替えを許可する。
        aiVector3D previousScale, currentScale;
        aiVector3D previousPosition, currentPosition;
        aiQuaternion previousRotation, currentRotation;
        m_previousWorld.Decompose(previousScale, previousRotation, previousPosition);
        world.Decompose(currentScale, currentRotation, currentPosition);
        const float rotationDot =
            std::abs(previousRotation.w * currentRotation.w + previousRotation.x * currentRotation.x +
                     previousRotation.y * currentRotation.y + previousRotation.z * currentRotation.z);
        const bool turning = !reset && rotationDot < TurningRotationDot;

        int group = -1; // -1なら踏み替え不要。0または1なら該当する3本組を動かす。
        if (moving || turning) {
            for (int attempt = 0; attempt < TripodCount && group < 0; ++attempt) {
                const int candidate = (m_nextTripod + attempt) % TripodCount;
                for (size_t index = candidate; index < m_feet.size(); index += TripodCount) {
                    const auto home = world * m_legs[index].home;
                    const auto behind = Horizontal(home - m_feet[index].position);
                    const bool needsStep = moving ? behind * (velocity / speed) > m_settings.stepDistance
                                                  : behind.Length() > m_settings.stepDistance;
                    const bool continuousWalk = moving && speed * stepSeconds >= m_settings.stepDistance;

                    // 歩き始めは片方を先に出す。通常歩行は交互に動かし続ける。
                    if (needsStep || continuousWalk || (moving && !m_wasMoving)) {
                        group = candidate;
                        break;
                    }
                }
            }
        }

        if (group >= 0) {
            const bool shortStep = moving && (!m_wasMoving || m_turnSteps > 0);
            const float duration = shortStep ? stepSeconds * ShortStepDurationRatio : stepSeconds;
            BeginTripodStep(group, world, groundY, velocity, duration, stepSeconds);
            m_nextTripod = (group + 1) % TripodCount;
            if (m_turnSteps > 0) {
                --m_turnSteps;
            }
        }
    }

    m_wasMoving = moving;
    m_previousVelocity = velocity;
    UpdateBodyMotion(dt, world, grounded, speed, acceleration);
    Solve(rig, world);
}

bool RobotSpiderIK::UpdateSwingingFeet(float dt, const aiMatrix4x4& world, float groundY, bool grounded,
                                       bool moving, const aiVector3D& velocity, float stepSeconds) {
    bool stillSwinging = false; // この更新を終えても空中に残っている足があるか。
    for (size_t index = 0; index < m_feet.size(); ++index) {
        auto& foot = m_feet[index];
        if (!foot.swinging) {
            continue; // 接地中の足はワールド座標に固定する。
        }

        const float remaining = foot.duration * (1.0f - foot.progress);
        if (moving) {
            // 方向転換時は空中の足の着地先だけを追従させる。
            const float leadSeconds = (std::max)(0.0f, remaining - dt) + stepSeconds * LandingLeadRatio;
            foot.target = world * m_legs[index].home + velocity * leadSeconds;
            foot.target.y = groundY;
        }

        // 現在位置・速度から着地点までをHermite補間する。
        // 着地点が変わっても、過去の出発点から計算し直さないので空中で足が飛ばない。
        const float flightTime = (std::max)(remaining, dt); // 今の位置から着地までの残り秒数。
        const float u = (std::min)(1.0f, dt / flightTime); // 残りの軌道のうち今回進む割合。
        const float u2 = u * u;
        const float u3 = u2 * u;
        const auto current = foot.position - foot.arcOffset; // 外側の弧を除いた位置。
        const auto tangent = foot.velocity * flightTime; // Hermite補間の始点の接線。

        // 2,3,6などはHermite基底とその微分の係数。調整値ではなく補間式そのもの。
        foot.position =
            current * (2 * u3 - 3 * u2 + 1) + tangent * (u3 - 2 * u2 + u) + foot.target * (-2 * u3 + 3 * u2);
        foot.velocity =
            (current * (6 * u2 - 6 * u) + tangent * (3 * u2 - 4 * u + 1) + foot.target * (-6 * u2 + 6 * u)) /
            flightTime;
        foot.velocity.y = 0.0f;
        foot.progress = (std::min)(1.0f, foot.progress + dt / foot.duration);

        const float progress = foot.progress;
        const float smoothProgress = progress * progress * (3 - 2 * progress); // 両端の速度が0の補間。
        const float arcHeight = 4 * progress * (1 - progress); // 開始・着地が0、途中が1になる放物線。
        foot.position.y =
            foot.start.y + (foot.target.y - foot.start.y) * smoothProgress + foot.liftHeight * arcHeight;

        // 空中の足だけ外側へ回す。停止時の着地では弧を加えず、その場の真下へ下ろす。
        foot.arcOffset =
            foot.liftHeight > 0.0f ? foot.outward * (m_settings.swingOutward * arcHeight) : aiVector3D();
        foot.position += foot.arcOffset;

        if (progress >= 1.0f) {
            foot.swinging = false;
            foot.arcOffset = {};
            if (grounded) {
                m_body.velocity.y -= FootfallBodyImpulse;
            }
        } else {
            stillSwinging = true;
        }
    }

    return stillSwinging;
}

void RobotSpiderIK::BeginTripodStep(int group, const aiMatrix4x4& world, float groundY,
                                    const aiVector3D& velocity, float duration, float stepSeconds) {
    // 着地までの移動に加え、次の支持期間の途中まで本体が進む量を先読みする。
    const auto lead = velocity * (duration + stepSeconds * LandingLeadRatio);
    for (size_t index = group; index < m_feet.size(); index += TripodCount) {
        auto& foot = m_feet[index];
        foot.start = foot.position;
        foot.target = world * m_legs[index].home + lead;
        foot.target.y = groundY;
        foot.progress = 0.0f;
        foot.duration = duration;
        foot.liftHeight = m_settings.stepHeight;
        foot.swinging = true;
        foot.velocity = {};

        const auto base = Position(m_rest[m_legs[index].joints[Coxa]]);
        foot.outward = Horizontal(aiMatrix3x3(world) * (m_legs[index].home - base)).Normalize();
        foot.arcOffset = {};
        ++foot.stepNumber;
    }
}

void RobotSpiderIK::UpdateBodyMotion(float dt, const aiMatrix4x4& world, bool grounded, float speed,
                                     const aiVector3D& acceleration) {
    auto targetOffset = Limited(-m_filteredVelocity * BodyLagSeconds, MaxBodyLagDistance);
    const float movementRatio = (std::min)(speed / BodySinkReferenceSpeed, 1.0f);
    targetOffset.y = grounded ? RestBodyOffsetY - movementRatio * MovingBodySink : AirborneBodyOffsetY;
    m_body.Update(targetOffset, dt, m_settings.bodyFrequency, m_settings.bodyDamping);
    m_body.value.y = std::clamp(m_body.value.y, MinBodyOffsetY, MaxBodyOffsetY);

    aiMatrix3x3 inverseRotation(world);
    inverseRotation.Inverse();
    auto localDrive =
        inverseRotation * (m_filteredVelocity * VelocityTiltWeight + acceleration * AccelerationTiltWeight);

    // 本体は均等拡大なので、X軸の長さからモデル倍率を取得できる。
    // 逆行列に含まれる逆倍率を打ち消し、向きだけを変換する。倍率の直書きを避ける。
    const float modelScale = aiVector3D(world.a1, world.b1, world.c1).Length();
    localDrive *= modelScale;
    const aiVector3D targetTilt(std::clamp(localDrive.z, -MaxBodyTiltRadians, MaxBodyTiltRadians), 0.0f,
                                std::clamp(-localDrive.x, -MaxBodyTiltRadians, MaxBodyTiltRadians));
    m_tilt.Update(targetTilt, dt, m_settings.tiltFrequency, m_settings.tiltDamping);
}

void RobotSpiderIK::TransformBranch(RobotAnimationRig& rig, int root, const aiMatrix4x4& transform) {
    // 関節とその子孫に同じ変換を掛ける。隣の脚や、別の枝の骨は動かさない。
    for (size_t index = root; index < rig.m_nodes.size(); ++index) {
        int parent = static_cast<int>(index);
        while (parent >= 0 && parent != root) {
            parent = rig.m_nodes[parent].parent;
        }

        if (parent == root) {
            rig.m_globals[index] = transform * rig.m_globals[index];
        }
    }
}

void RobotSpiderIK::Solve(RobotAnimationRig& rig, const aiMatrix4x4& world) {
    auto inverseWorld = world;
    inverseWorld.Inverse();

    // まず胴体の揺れと高さを反映する。その後、固定された接地点へ各脚を届かせる。
    aiMatrix4x4 pitch;
    aiMatrix4x4 roll;
    aiMatrix4x4::RotationX(m_tilt.value.x, pitch);
    aiMatrix4x4::RotationZ(m_tilt.value.z, roll);
    const int root = rig.FindNode("Root_M");
    auto body = Around(roll * pitch, Position(m_rest[root]));
    const auto offset = aiMatrix3x3(inverseWorld) * (m_body.value + aiVector3D(0, m_settings.bodyHeight, 0));
    body.a4 += offset.x;
    body.b4 += offset.y;
    body.c4 += offset.z;
    rig.m_globals = m_rest;
    TransformBranch(rig, root, body);

    for (size_t index = 0; index < m_legs.size(); ++index) {
        const auto& leg = m_legs[index];
        const int coxa = leg.joints[Coxa];
        const int hip = leg.joints[Hip];
        const int knee = leg.joints[Knee];
        const int ankle = leg.joints[Ankle];
        const auto target = inverseWorld * m_feet[index].position + leg.ankleOffset;

        // 付け根を水平に回し、脚全体を着地点の方向へ向ける。
        const auto coxaPosition = Position(rig.m_globals[coxa]);
        const auto oldDirection = Horizontal(Position(rig.m_globals[ankle]) - coxaPosition);
        const auto newDirection = Horizontal(target - coxaPosition);
        if (oldDirection.SquareLength() > IKVectorSquaredEpsilon &&
            newDirection.SquareLength() > IKVectorSquaredEpsilon) {
            const float angle =
                std::atan2(newDirection.x, newDirection.z) - std::atan2(oldDirection.x, oldDirection.z);
            aiMatrix4x4 yaw;
            aiMatrix4x4::RotationY(angle, yaw);
            TransformBranch(rig, coxa, Around(yaw, coxaPosition));
        }

        // Hip→Knee→Ankleを長さの変わらない2本の棒として解く。
        const auto hipPosition = Position(rig.m_globals[hip]);
        const auto kneePosition = Position(rig.m_globals[knee]);
        const auto anklePosition = Position(rig.m_globals[ankle]);
        const float upperLength = (kneePosition - hipPosition).Length();
        const float lowerLength = (anklePosition - kneePosition).Length();
        auto direction = target - hipPosition;
        const float distance =
            std::clamp(direction.Length(), std::abs(upperLength - lowerLength) + JointReachMargin,
                       upperLength + lowerLength - JointReachMargin);

        if (direction.SquareLength() < IKVectorSquaredEpsilon) {
            direction = anklePosition - hipPosition;
        }
        direction.Normalize();

        // 膝を素材と同じ側へ曲げるための補助方向。逆向きに折れるのを防ぐ。
        auto pole = kneePosition - hipPosition;
        pole -= direction * (pole * direction);
        if (pole.SquareLength() < IKVectorSquaredEpsilon) {
            pole = aiVector3D(0, 1, 0) - direction * direction.y;
        }
        pole.Normalize();

        // 余弦定理で膝位置を求める。alongは目標方向への距離、heightはそこから膝までの高さ。
        const float along =
            (upperLength * upperLength - lowerLength * lowerLength + distance * distance) / (2 * distance);
        const float height = std::sqrt((std::max)(0.0f, upperLength * upperLength - along * along));
        const auto solvedKnee = hipPosition + direction * along + pole * height;
        const auto solvedAnkle = hipPosition + direction * distance;
        TransformBranch(rig, hip, Align(kneePosition - hipPosition, solvedKnee - hipPosition, hipPosition));

        const auto kneePivot = Position(rig.m_globals[knee]);
        TransformBranch(
            rig, knee, Align(Position(rig.m_globals[ankle]) - kneePivot, solvedAnkle - kneePivot, kneePivot));

        // 胴体が傾いても足裏は水平に保つ。位置はIKの結果、向きは基準姿勢を使う。
        auto levelAnkle = m_rest[ankle];
        const auto solvedAnklePosition = Position(rig.m_globals[ankle]);
        levelAnkle.a4 = solvedAnklePosition.x;
        levelAnkle.b4 = solvedAnklePosition.y;
        levelAnkle.c4 = solvedAnklePosition.z;
        auto inverseAnkle = rig.m_globals[ankle];
        inverseAnkle.Inverse();
        TransformBranch(rig, ankle, levelAnkle * inverseAnkle);
        m_solvedContacts[index] = world * (rig.m_globals[ankle] * leg.localContact);
    }

    // 最後に上半身だけ照準へ向ける。脚の接地点には影響させない。
    rig.ApplyUpperBodyAim();
}
