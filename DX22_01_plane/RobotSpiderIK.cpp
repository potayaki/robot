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
    *this = RobotSpiderIK{};
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
        leg.home = contact;
        leg.ankleOffset = Position(m_rest[ankle]) - contact;
        auto inverse = m_rest[ankle]; inverse.Inverse();
        leg.localContact = inverse * contact;
    }
    m_initialized = true;
    return true;
}

void RobotSpiderIK::Reset()
{
    m_tracking = false;
    m_body = {}; m_tilt = {}; m_filteredVelocity = {};
    m_nextTripod = 0;
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
    bool swinging = false;
    for (auto& foot : m_feet)
    {
        if (!foot.swinging) continue;
        foot.progress = (std::min)(1.0f, foot.progress + dt / m_stepDuration);
        const float t = foot.progress;
        const float smooth = t * t * (3 - 2 * t);
        foot.position = foot.start + (foot.target - foot.start) * smooth;
        foot.position.y += settings.stepHeight * 4 * t * (1 - t);
        if (t >= 1)
        {
            foot.swinging = false;
            if (grounded) m_body.velocity.y -= 0.35f;
        }
        else swinging = true;
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
        // ★追加：速度がほとんど無い（停止中＝Idle状態）なら、勝手にステップを始めない！
         // これにより、止まった瞬間に足が初期位置へ勝手に戻るのを防ぎます。
        if (speed < 0.1f) {
            // 停止中は現在の足の位置をそのままキープする
        }
        else {
            // 従来の歩行ステップ処理（移動中のみ実行）
            int group = -1;
            for (int attempt = 0; attempt < 2 && group < 0; ++attempt) {
                const int candidate = (m_nextTripod + attempt) % 2;
                for (size_t i = candidate; i < m_feet.size(); i += 2) {
                    const auto home = world * m_legs[i].home;
                    if (Horizontal(home - m_feet[i].position).Length() > settings.stepDistance || speed > 4.0f)                         {
 group = candidate; break; }
                        }
                    }
                    if (group >= 0) {
                        // ★ついでに、先ほど話題になった「バタつき防止」の下限値（0.08f）もここに反映しています
                        m_stepDuration = std::clamp(2.7f / (speed + 8.0f), 0.08f, 0.22f);

                        // 高速移動時の歩幅（lead）の上限も少し広げておくと安定します
                        const auto lead = Limited(velocity * (m_stepDuration * 1.5f), 8.0f);

                        for (size_t i = group; i < m_feet.size(); i += 2) {
                            auto& foot = m_feet[i];
                            foot.start = foot.position;
                            foot.target = world * m_legs[i].home + lead;
                            foot.target.y = groundY;
                            foot.progress = 0; foot.swinging = true;
                            ++foot.stepNumber;
                        }
                        m_nextTripod = 1 - group;
                    }
                }
    }

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
    const auto offset = aiMatrix3x3(inverse) * m_body.value;
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
