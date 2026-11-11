#include "RobotSpiderIK.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265359f;
    aiVector3D Position(const aiMatrix4x4& m) { return m * aiVector3D(); }
    aiVector3D Horizontal(aiVector3D v) { v.y = 0; return v; }
    aiVector3D Limited(aiVector3D v, float maximum)
    {
        const float length = v.Length();
        return length > maximum ? v * (maximum / length) : v;
    }
    aiMatrix4x4 Around(aiMatrix4x4 rotation, aiVector3D pivot)
    {
        const auto offset = pivot - rotation * pivot;
        rotation.a4 = offset.x; rotation.b4 = offset.y; rotation.c4 = offset.z;
        return rotation;
    }
    aiMatrix4x4 Align(aiVector3D from, aiVector3D to, aiVector3D pivot)
    {
        if (from.SquareLength() < 1e-8f || to.SquareLength() < 1e-8f) return aiMatrix4x4();
        from.Normalize(); to.Normalize();
        const float cosine = std::clamp(from * to, -1.0f, 1.0f);
        auto axis = from ^ to;
        if (axis.SquareLength() < 1e-8f)
        {
            if (cosine > 0) return aiMatrix4x4();
            axis = from ^ aiVector3D(0, 1, 0);
            if (axis.SquareLength() < 1e-8f) axis = from ^ aiVector3D(1, 0, 0);
        }
        axis.Normalize();
        return Around(aiMatrix4x4(aiQuaternion(axis, std::acos(cosine)).GetMatrix()), pivot);
    }
}

void RobotSpiderIK::Spring::Update(aiVector3D target, float dt, float frequency, float damping)
{
    // Small integration steps keep the damped second-order response stable at low FPS.
    const float omega = 2 * Pi * (std::max)(0.1f, frequency);
    const int steps = (std::max)(1, static_cast<int>(std::ceil(dt / (1.0f / 240))));
    const float h = dt / steps;
    for (int i = 0; i < steps; ++i)
    {
        velocity += ((target - value) * (omega * omega) - velocity * (2 * damping * omega)) * h;
        value += velocity * h;
    }
}

bool RobotSpiderIK::Init(const RobotAnimationRig& rig)
{
    const auto tuning = settings;
    *this = RobotSpiderIK{};
    settings = tuning;
    m_rest = rig.m_globals;
    const char* prefixes[] = {"BackLeg", "MiddleLeg", "FrontLeg"};
    for (int side = 0; side < 2; ++side) for (int row = 0; row < 3; ++row)
    {
        auto& leg = m_legs[side * 3 + row];
        for (int j = 0; j < 4; ++j)
        {
            const auto name = std::string(prefixes[row]) + std::to_string(j + 1) + (side == 0 ? "_R" : "_L");
            leg.joints[j] = rig.FindNode(name);
            if (leg.joints[j] < 0) return false;
        }
        // The End markers sit below the visible soles. Use the actual foot geometry.
        const int ankle = leg.joints[3];
        float bottom = 1e9f;
        for (const auto& part : rig.m_parts) for (const auto& v : part.vertices)
            if (v.node == ankle) bottom = (std::min)(bottom, (m_rest[ankle] * v.position).y);
        if (bottom == 1e9f) return false;
        aiVector3D contact;
        int count = 0;
        for (const auto& part : rig.m_parts) for (const auto& v : part.vertices)
        {
            if (v.node != ankle) continue;
            const auto p = m_rest[ankle] * v.position;
            if (p.y <= bottom + 0.5f) { contact += p; ++count; }
        }
        contact /= static_cast<float>(count);
        contact.y = bottom;
        leg.restContact = contact;
        leg.home = GetHome(leg);
        leg.ankleOffset = Position(m_rest[ankle]) - contact;
        auto inverse = m_rest[ankle]; inverse.Inverse();
        leg.localContact = inverse * contact;
    }
    m_initialized = true;
    return true;
}

aiVector3D RobotSpiderIK::GetHome(const Leg& leg) const
{
    // 毎回FBXの元の接地点から計算する。スライダーを往復しても倍率が累積しない。
    const auto base = Position(m_rest[leg.joints[0]]);
    auto home = leg.restContact;
    home.x = base.x + (home.x - base.x) * settings.stanceWidth;
    home.z = base.z + (home.z - base.z) * settings.stanceWidth;
    return home;
}

void RobotSpiderIK::ApplySettings(RobotAnimationRig& rig, const Settings& requested)
{
    settings = requested;
    const Settings defaults;
    const auto valid = [](float value, float fallback, float low, float high)
    {
        return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
    };
    settings.strideLength = valid(settings.strideLength, defaults.strideLength, 8.0f, 24.0f);
    settings.stanceWidth = valid(settings.stanceWidth, defaults.stanceWidth, 0.65f, 1.05f);
    settings.bodyHeight = valid(settings.bodyHeight, defaults.bodyHeight, -0.5f, 2.5f);
    settings.stepHeight = valid(settings.stepHeight, defaults.stepHeight, 0.2f, 2.0f);
    if (!m_initialized) return;

    for (size_t i = 0; i < m_legs.size(); ++i)
    {
        auto& leg = m_legs[i];
        const auto previousHome = leg.home;
        leg.home = GetHome(leg);
        if (!m_tracking) continue;

        // 開き具合を変更した瞬間だけ接地点を外側／内側へ動かす。
        // 歩行中の前後位置や踏み出しの進み具合は維持し、停止後にIdleへ戻さない。
        const auto shift = Horizontal(aiMatrix3x3(m_previousWorld) * (leg.home - previousHome));
        m_feet[i].position += shift;
        m_feet[i].start += shift;
        m_feet[i].target += shift;
    }
    if (m_tracking) Solve(rig, m_previousWorld);
}

float RobotSpiderIK::GetContactError() const
{
    if (!m_tracking) return 0.0f;
    float maximum = 0.0f;
    for (size_t i = 0; i < m_feet.size(); ++i)
        maximum = (std::max)(maximum, (m_solvedContacts[i] - m_feet[i].position).Length());
    return maximum;
}

void RobotSpiderIK::Reset()
{
    m_tracking = false;
    m_body = {}; m_tilt = {}; m_filteredVelocity = {};
    m_nextTripod = 0;
    m_wasMoving = false;
    m_previousVelocity = {};
    m_turnSteps = 0;
}

void RobotSpiderIK::Update(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world,
                          float groundY, bool grounded)
{
    if (!m_initialized || dt <= 0) return;
    if (!m_tracking || (Position(world) - m_previousPosition).Length() > 35 || dt > 0.25f)
    {
        Step(rig, dt, world, groundY, grounded);
        m_previousWorld = world;
        return;
    }
    // Plan footsteps at a fixed maximum interval as well as integrating springs.
    // At 30 FPS a fast step can span a whole frame; substeps preserve its timing.
    aiVector3D oldScale, newScale, oldPosition, newPosition;
    aiQuaternion oldRotation, newRotation;
    m_previousWorld.Decompose(oldScale, oldRotation, oldPosition);
    world.Decompose(newScale, newRotation, newPosition);
    const int steps = (std::max)(1, static_cast<int>(std::ceil(dt * 240)));
    for (int step = 1; step <= steps; ++step)
    {
        const float alpha = static_cast<float>(step) / steps;
        aiQuaternion rotation;
        aiQuaternion::Interpolate(rotation, oldRotation, newRotation, alpha);
        const aiMatrix4x4 interpolated(oldScale + (newScale - oldScale) * alpha,
            rotation.Normalize(), oldPosition + (newPosition - oldPosition) * alpha);
        Step(rig, dt / steps, interpolated, groundY, grounded);
    }
    m_previousWorld = world;
}

void RobotSpiderIK::Step(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world,
                        float groundY, bool grounded)
{
    const auto position = Position(world);
    const auto displacement = position - m_previousPosition;
    // Teleports, restarts and debug repositioning must not stretch planted feet across the map.
    const bool reset = !m_tracking || displacement.Length() > 35.0f || dt > 0.25f;
    if (reset)
    {
        Reset(); m_tracking = true;
        m_previousPosition = position;
        for (size_t i = 0; i < m_feet.size(); ++i)
        {
            m_feet[i] = {};
            m_feet[i].position = world * m_legs[i].home;
            if (grounded) m_feet[i].position.y = groundY;
        }
        m_grounded = grounded;
    }
    const auto velocity = reset ? aiVector3D() : Horizontal(displacement) / dt;
    const float speed = velocity.Length();
    const auto previousVelocity = m_filteredVelocity;
    m_filteredVelocity += (velocity - m_filteredVelocity) * (1 - std::exp(-10 * dt));
    const auto acceleration = Limited((m_filteredVelocity - previousVelocity) / dt, 350.0f);
    m_previousPosition = position;

    if (grounded && !m_grounded)
    {
        for (size_t i = 0; i < m_feet.size(); ++i)
        {
            m_feet[i] = {};
            m_feet[i].position = world * m_legs[i].home;
            m_feet[i].position.y = groundY;
        }
        m_body.velocity.y -= 4.0f;
    }
    m_grounded = grounded;
    const bool moving = grounded && speed > 0.1f;
    const float stepSeconds = std::clamp(settings.strideLength / (2.0f * (std::max)(speed, 1.0f)),
        settings.minStepSeconds, settings.maxStepSeconds);

    const float previousSpeed = m_previousVelocity.Length();
    const bool directionChanged = moving && m_wasMoving && previousSpeed > 0.1f &&
        (velocity * m_previousVelocity < speed * previousSpeed * 0.85f || speed > previousSpeed * 1.2f);
    if (directionChanged)
    {
        // 大股の途中で横・後ろへ切り返すと、地面に残した脚が伸び切る。
        // このときだけ早めに着地して次の3本へ渡し、通常歩行では長い周期を保つ。
        m_turnSteps = 2;
        for (auto& foot : m_feet)
        {
            if (!foot.swinging) continue;
            const float phaseLeft = 1.0f - foot.progress;
            if (phaseLeft > 0.0001f)
                foot.duration = (std::min)(foot.duration, 0.06f / phaseLeft);
        }
    }

    // 入力を離したら、浮いている足だけ現在の真下へ静かに下ろす。
    // 先読みした遠い着地点へ足を投げ出したり、Idleの基準位置へ戻したりしない。
    if (m_wasMoving && !moving && grounded)
    {
        for (auto& foot : m_feet)
        {
            if (!foot.swinging) continue;
            foot.start = foot.position;
            foot.target = foot.position;
            foot.target.y = groundY;
            foot.progress = 0.0f;
            foot.duration = 0.12f;
            foot.liftHeight = 0.0f;
            foot.velocity = {};
            foot.arcOffset = {};
        }
    }

    bool swinging = false;
    for (size_t i = 0; i < m_feet.size(); ++i)
    {
        auto& foot = m_feet[i];
        if (!foot.swinging) continue;
        const float remaining = foot.duration * (1.0f - foot.progress);
        if (moving)
        {
            // 方向転換時は空中の足の着地先だけを追従させ、接地中の足は動かさない。
            foot.target = world * m_legs[i].home + velocity *
                ((std::max)(0.0f, remaining - dt) + stepSeconds * 0.5f);
            foot.target.y = groundY;
        }

        // 現在位置・速度から着地点までをHermite補間する。
        // 着地点が変わっても元の出発点から再計算しないため、空中で足が飛ばない。
        const float flightTime = (std::max)(remaining, dt);
        const float u = (std::min)(1.0f, dt / flightTime);
        const float u2 = u * u, u3 = u2 * u;
        const auto current = foot.position - foot.arcOffset;
        const auto tangent = foot.velocity * flightTime;
        foot.position = current * (2 * u3 - 3 * u2 + 1) +
            tangent * (u3 - 2 * u2 + u) + foot.target * (-2 * u3 + 3 * u2);
        foot.velocity = (current * (6 * u2 - 6 * u) +
            tangent * (3 * u2 - 4 * u + 1) + foot.target * (-6 * u2 + 6 * u)) / flightTime;
        foot.velocity.y = 0.0f;
        foot.progress = (std::min)(1.0f, foot.progress + dt / foot.duration);
        const float t = foot.progress;
        const float smooth = t * t * (3 - 2 * t);
        foot.position.y = foot.start.y + (foot.target.y - foot.start.y) * smooth +
            foot.liftHeight * 4 * t * (1 - t);
        // 接地中の足は固定し、空中の足だけ外側へ弧を描かせる。
        // 停止時の着地では弧を加えず、現在の真下へ下ろす。
        foot.arcOffset = foot.liftHeight > 0.0f
            ? foot.outward * (settings.swingOutward * 4 * t * (1 - t)) : aiVector3D();
        foot.position += foot.arcOffset;
        if (t >= 1.0f)
        {
            foot.swinging = false;
            foot.arcOffset = {};
            if (grounded) m_body.velocity.y -= 0.35f;
        }
        else
        {
            swinging = true;
        }
    }

    if (!grounded)
    {
        for (size_t i = 0; i < m_feet.size(); ++i)
        {
            auto& foot = m_feet[i];
            foot.swinging = false;
            foot.position = world * m_legs[i].home + aiVector3D(0, 1.2f, 0);
        }
    }
    else if (!swinging)
    {
        // 停止中は接地点を保持する。基準姿勢との差だけでIdleへ足を戻さない。
        // 下半身そのものを回したときだけは、回転に合わせた踏み替えを許可する。
        aiVector3D previousScale, currentScale, previousPosition, currentPosition;
        aiQuaternion previousRotation, currentRotation;
        m_previousWorld.Decompose(previousScale, previousRotation, previousPosition);
        world.Decompose(currentScale, currentRotation, currentPosition);
        const float rotationDot = std::abs(previousRotation.w * currentRotation.w +
            previousRotation.x * currentRotation.x + previousRotation.y * currentRotation.y +
            previousRotation.z * currentRotation.z);
        const bool turning = !reset && rotationDot < 0.9999999f;

        int group = -1;
        if (moving || turning)
        {
            for (int attempt = 0; attempt < 2 && group < 0; ++attempt)
            {
                const int candidate = (m_nextTripod + attempt) % 2;
                for (size_t i = candidate; i < m_feet.size(); i += 2)
                {
                    const auto home = world * m_legs[i].home;
                    const auto behind = Horizontal(home - m_feet[i].position);
                    // 歩き始めは片側を先に出す。歩行中は既に前へ出した足を再び上げない。
                    const bool needsStep = moving
                        ? behind * (velocity / speed) > settings.stepDistance
                        : behind.Length() > settings.stepDistance;
                    const bool continuousWalk = moving && speed * stepSeconds >= settings.stepDistance;
                    if (needsStep || continuousWalk || (moving && !m_wasMoving))
                    {
                        group = candidate;
                        break;
                    }
                }
            }
        }

        if (group >= 0)
        {
            // 3本ずつ交互に動かすため、同じ足の一周期は踏み出し時間の約2倍。
            // 速度だけで短い周期を繰り返さず、希望する歩幅から所要時間を求める。
            // 歩き始めの最初の3本だけ半周期にして、反対側を地面に残しすぎない。
            const bool shortStep = moving && (!m_wasMoving || m_turnSteps > 0);
            const float duration = shortStep ? stepSeconds * 0.5f : stepSeconds;
            const auto lead = velocity * (duration + stepSeconds * 0.5f);
            for (size_t i = group; i < m_feet.size(); i += 2)
            {
                auto& foot = m_feet[i];
                foot.start = foot.position;
                foot.target = world * m_legs[i].home + lead;
                foot.target.y = groundY;
                foot.progress = 0.0f;
                foot.duration = duration;
                foot.liftHeight = settings.stepHeight;
                foot.swinging = true;
                foot.velocity = {};
                const auto base = Position(m_rest[m_legs[i].joints[0]]);
                foot.outward = Horizontal(aiMatrix3x3(world) * (m_legs[i].home - base)).Normalize();
                foot.arcOffset = {};
                ++foot.stepNumber;
            }
            m_nextTripod = 1 - group;
            if (m_turnSteps > 0) --m_turnSteps;
        }
    }
    m_wasMoving = moving;
    m_previousVelocity = velocity;

    auto targetOffset = Limited(-m_filteredVelocity * 0.012f, 1.0f);
    targetOffset.y = grounded ? -0.15f - (std::min)(speed / 60.0f, 1.0f) * 0.20f : 0.25f;
    m_body.Update(targetOffset, dt, settings.bodyFrequency, settings.bodyDamping);
    m_body.value.y = std::clamp(m_body.value.y, -1.2f, 0.8f);
    aiMatrix3x3 inverseRotation(world); inverseRotation.Inverse();
    auto localDrive = inverseRotation * (m_filteredVelocity * 0.0007f + acceleration * 0.00012f);
    // Remove the model's inverse scale (1/0.025) from this direction transform.
    localDrive *= 0.025f;
    aiVector3D targetTilt(std::clamp(localDrive.z, -0.10f, 0.10f), 0,
        std::clamp(-localDrive.x, -0.10f, 0.10f));
    m_tilt.Update(targetTilt, dt, settings.tiltFrequency, settings.tiltDamping);
    Solve(rig, world);
}

void RobotSpiderIK::TransformBranch(RobotAnimationRig& rig, int root, const aiMatrix4x4& transform)
{
    for (size_t i = root; i < rig.m_nodes.size(); ++i)
    {
        int parent = static_cast<int>(i);
        while (parent >= 0 && parent != root) parent = rig.m_nodes[parent].parent;
        if (parent == root) rig.m_globals[i] = transform * rig.m_globals[i];
    }
}

void RobotSpiderIK::Solve(RobotAnimationRig& rig, const aiMatrix4x4& world)
{
    auto inverse = world; inverse.Inverse();
    aiMatrix4x4 pitch, roll;
    aiMatrix4x4::RotationX(m_tilt.value.x, pitch);
    aiMatrix4x4::RotationZ(m_tilt.value.z, roll);
    const int root = rig.FindNode("Root_M");
    auto body = Around(roll * pitch, Position(m_rest[root]));
    const auto offset = aiMatrix3x3(inverse) * (m_body.value + aiVector3D(0, settings.bodyHeight, 0));
    body.a4 += offset.x; body.b4 += offset.y; body.c4 += offset.z;
    rig.m_globals = m_rest;
    TransformBranch(rig, root, body);

    for (size_t i = 0; i < m_legs.size(); ++i)
    {
        const auto& leg = m_legs[i];
        const int coxa = leg.joints[0], hip = leg.joints[1], knee = leg.joints[2], ankle = leg.joints[3];
        const auto target = inverse * m_feet[i].position + leg.ankleOffset;
        auto c = Position(rig.m_globals[coxa]);
        auto oldDirection = Horizontal(Position(rig.m_globals[ankle]) - c);
        auto newDirection = Horizontal(target - c);
        if (oldDirection.SquareLength() > 1e-6f && newDirection.SquareLength() > 1e-6f)
        {
            const float angle = std::atan2(newDirection.x, newDirection.z) - std::atan2(oldDirection.x, oldDirection.z);
            aiMatrix4x4 yaw; aiMatrix4x4::RotationY(angle, yaw);
            TransformBranch(rig, coxa, Around(yaw, c));
        }
        const auto h = Position(rig.m_globals[hip]);
        const auto k = Position(rig.m_globals[knee]);
        const auto a = Position(rig.m_globals[ankle]);
        const float upper = (k - h).Length(), lower = (a - k).Length();
        auto direction = target - h;
        const float distance = std::clamp(direction.Length(), std::abs(upper - lower) + 0.01f, upper + lower - 0.01f);
        if (direction.SquareLength() < 1e-6f) direction = a - h;
        direction.Normalize();
        // Keep the knee on the rest-pose side of the chain, avoiding inverted knees.
        auto pole = k - h;
        pole -= direction * (pole * direction);
        if (pole.SquareLength() < 1e-6f) pole = aiVector3D(0, 1, 0) - direction * direction.y;
        pole.Normalize();
        const float along = (upper * upper - lower * lower + distance * distance) / (2 * distance);
        const float height = std::sqrt((std::max)(0.0f, upper * upper - along * along));
        const auto solvedKnee = h + direction * along + pole * height;
        const auto solvedAnkle = h + direction * distance;
        TransformBranch(rig, hip, Align(k - h, solvedKnee - h, h));
        const auto pivot = Position(rig.m_globals[knee]);
        TransformBranch(rig, knee, Align(Position(rig.m_globals[ankle]) - pivot, solvedAnkle - pivot, pivot));
        // Keep each rigid sole level while the body leans over planted feet.
        auto level = m_rest[ankle];
        const auto anklePosition = Position(rig.m_globals[ankle]);
        level.a4 = anklePosition.x; level.b4 = anklePosition.y; level.c4 = anklePosition.z;
        auto oldInverse = rig.m_globals[ankle]; oldInverse.Inverse();
        TransformBranch(rig, ankle, level * oldInverse);
        m_solvedContacts[i] = world * (rig.m_globals[ankle] * leg.localContact);
    }
    rig.ApplyUpperBodyAim();
}
