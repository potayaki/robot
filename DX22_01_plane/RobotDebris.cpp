/// @file RobotDebris.cpp
/// @brief 現在のロボット姿勢から作る破片と、落下・回転・着地。

#include "RobotDebris.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace {
    // 速度は距離/秒、重力は距離/秒²。表示FPSに応じたフレーム数では管理しない。
    constexpr float Gravity = 75.0f;
    constexpr float HorizontalSpeed = 28.0f;
    constexpr float UpwardSpeed = 32.0f;
    constexpr float Bounce = 0.28f;
    constexpr double PhysicsStepSeconds = 1.0 / 120.0;

    void ExpandBounds(const aiVector3D& point, aiVector3D& minimum, aiVector3D& maximum) {
        for (unsigned axis = 0; axis < 3; ++axis) {
            minimum[axis] = (std::min)(minimum[axis], point[axis]);
            maximum[axis] = (std::max)(maximum[axis], point[axis]);
        }
    }
} // namespace

void RobotDebris::Start(const RobotAnimationRig& rig, const aiMatrix4x4& modelToWorld, float groundY) {
    // HPが0の更新が続いても、最初の1回だけ分解する。
    if (IsActive() || rig.GetMatrices().empty()) {
        return;
    }
    m_groundY = groundY;

    const auto& pose = rig.GetMatrices();
    std::vector<std::vector<RobotAnimationRig::Vertex>> groups(pose.size());
    std::vector<aiMatrix4x4> transforms(pose.size());
    std::vector<aiMatrix3x3> normalTransforms(pose.size());
    for (size_t i = 0; i < pose.size(); ++i) {
        transforms[i] = modelToWorld * pose[i];
        normalTransforms[i] = aiMatrix3x3(transforms[i]);
        normalTransforms[i].Inverse().Transpose();
    }

    const float limit = (std::numeric_limits<float>::max)();
    aiVector3D allMin(limit), allMax(-limit);
    for (const auto& part : rig.GetParts()) {
        for (size_t triangle = 0; triangle + 2 < part.vertices.size(); triangle += 3) {
            const auto& a = part.vertices[triangle];
            const auto& b = part.vertices[triangle + 1];
            const auto& c = part.vertices[triangle + 2];
            const unsigned owner = b.node == c.node ? b.node : a.node;

            // 関節をまたぐ三角形も3頂点まとめて同じ破片へ入れる。
            // 骨ごとに行列だけを飛ばすと、頂点同士が離れて長い膜のように伸びてしまう。
            for (size_t corner = 0; corner < 3; ++corner) {
                auto vertex = part.vertices[triangle + corner];
                vertex.position = transforms[vertex.node] * vertex.position;
                vertex.normal = (normalTransforms[vertex.node] * vertex.normal).Normalize();
                ExpandBounds(vertex.position, allMin, allMax);
                groups[owner].push_back(vertex);
            }
        }
    }

    const aiVector3D blastCenter = (allMin + allMax) * 0.5f;
    // 演出用の乱数を分離し、敵の出現・報酬抽選の乱数を消費しない。
    std::mt19937 random(0x524F424F);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    for (auto& group : groups) {
        if (group.empty()) {
            continue;
        }
        aiVector3D minimum(limit), maximum(-limit);
        for (const auto& vertex : group) {
            ExpandBounds(vertex.position, minimum, maximum);
        }

        Fragment fragment;
        fragment.position = (minimum + maximum) * 0.5f;
        fragment.firstVertex = m_vertices.size();
        fragment.vertexCount = group.size();
        const auto fragmentIndex = static_cast<unsigned>(m_fragments.size());
        for (auto vertex : group) {
            vertex.position -= fragment.position;
            fragment.radius = (std::max)(fragment.radius, vertex.position.Length());
            vertex.node = fragmentIndex; // 死亡表示では骨番号の欄を破片番号として使う。
            m_vertices.push_back(vertex);
        }

        aiVector3D outward = fragment.position - blastCenter;
        outward.y = 0.0f;
        if (outward.SquareLength() < 0.001f) {
            const float angle = random01(random) * 6.2831853f;
            outward = aiVector3D(std::cos(angle), 0.0f, std::sin(angle));
        }
        outward.Normalize();
        fragment.velocity = outward * (HorizontalSpeed + 18.0f * random01(random));
        fragment.velocity.y = UpwardSpeed + 20.0f * random01(random);
        fragment.spinAxis = aiVector3D(random01(random) - 0.5f, 0.6f, random01(random) - 0.5f).Normalize();
        fragment.spinSpeed = 2.0f + 5.0f * random01(random);
        m_fragments.push_back(fragment);
    }
    m_matrices.resize(m_fragments.size());
    UpdateMatrices();
}

void RobotDebris::Update(float deltaSeconds) {
    if (!IsActive() || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) {
        return;
    }

    // 実測した秒数を貯めて小分けに進める。低FPSでも接地を飛び越さず、一時停止も反映する。
    m_pendingSeconds += deltaSeconds;
    while (m_pendingSeconds + 1.0e-9 >= PhysicsStepSeconds) {
        Step(static_cast<float>(PhysicsStepSeconds));
        m_pendingSeconds -= PhysicsStepSeconds;
    }
    UpdateMatrices();
}

void RobotDebris::Step(float seconds) {
    for (auto& fragment : m_fragments) {
        if (fragment.resting) {
            continue;
        }

        fragment.position += fragment.velocity * seconds;
        fragment.position.y -= 0.5f * Gravity * seconds * seconds;
        fragment.velocity.y -= Gravity * seconds;
        const aiQuaternion spin(fragment.spinAxis, fragment.spinSpeed * seconds);
        fragment.rotation = (spin * fragment.rotation).Normalize();

        // 地面から十分遠い破片は、頂点を調べる接地処理を省略する。
        if (fragment.position.y - fragment.radius > m_groundY) {
            continue;
        }
        const aiMatrix3x3 rotation = fragment.rotation.GetMatrix();
        float lowest = (std::numeric_limits<float>::max)();
        for (size_t i = fragment.firstVertex; i < fragment.firstVertex + fragment.vertexCount; ++i) {
            const auto& point = m_vertices[i].position;
            const float y = rotation.b1 * point.x + rotation.b2 * point.y + rotation.b3 * point.z;
            lowest = (std::min)(lowest, y);
        }
        if (fragment.position.y + lowest >= m_groundY) {
            continue;
        }

        // 実際の頂点の最下部を床に合わせる。大きな胴体も脚の先端も床へ沈めない。
        fragment.position.y = m_groundY - lowest;
        if (fragment.velocity.y < 0.0f) {
            fragment.velocity.y *= -Bounce;
        }
        fragment.velocity.x *= 0.55f;
        fragment.velocity.z *= 0.55f;
        fragment.spinSpeed *= 0.6f;

        const float horizontalSquared =
            fragment.velocity.x * fragment.velocity.x + fragment.velocity.z * fragment.velocity.z;
        if (fragment.velocity.y < 1.0f && horizontalSquared < 1.0f && fragment.spinSpeed < 0.3f) {
            // 静止した残骸はその位置に残し、以後の物理計算を止める。
            fragment.velocity = aiVector3D(0, 0, 0);
            fragment.resting = true;
        }
    }
}

void RobotDebris::UpdateMatrices() {
    for (size_t i = 0; i < m_fragments.size(); ++i) {
        const auto& fragment = m_fragments[i];
        m_matrices[i] = aiMatrix4x4(aiVector3D(1, 1, 1), fragment.rotation, fragment.position);
    }
}

void RobotDebris::Reset() {
    *this = RobotDebris{};
}
