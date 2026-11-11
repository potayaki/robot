#include "../DX22_01_plane/RobotAnimationRig.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <utility>

float Difference(const std::vector<aiMatrix4x4>& a, const std::vector<aiMatrix4x4>& b)
{
    float sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        for (unsigned r = 0; r < 4; ++r)
            for (unsigned c = 0; c < 4; ++c) sum += std::abs(a[i][r][c] - b[i][r][c]);
    return sum;
}

void CheckUpperBodyAim(const RobotAnimationRig& original)
{
    // 実際のFBXから上半身の枝を特定し、全頂点の位置を比較する。
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const auto scene = importer.ReadFile("DX22_01_plane/assets/model/Player/Player.fbx",
        aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);
    assert(scene && scene->mRootNode);
    std::vector<bool> upperBody;
    size_t top = 0;
    const std::function<void(const aiNode*, bool)> visit = [&](const aiNode* node, bool upper)
    {
        if (std::string(node->mName.C_Str()) == "Top_M")
        {
            top = upperBody.size();
            upper = true;
        }
        upperBody.push_back(upper);
        for (unsigned i = 0; i < node->mNumChildren; ++i) visit(node->mChildren[i], upper);
    };
    visit(scene->mRootNode, false);
    assert(top > 0 && upperBody.size() == original.GetMatrices().size());

    // 下半身の拡大後も上半身・武器は平行移動だけで、全頂点間の距離を保つ。
    // 各歩行クリップで再評価しても倍率が消えたり、二重に掛かったりしないことを確認する。
    for (auto motion : {RobotMotion::Idle, RobotMotion::Forward, RobotMotion::Left})
    {
        auto normal = original, enlarged = original;
        enlarged.SetLowerBodyScale(1.40f);
        for (int frame = 0; frame < 20; ++frame)
        {
            normal.Update(motion, 1.0f / 60);
            enlarged.Update(motion, 1.0f / 60);
        }
        const auto offset = (normal.GetMatrices()[top] * aiVector3D()) * 0.40f;
        for (const auto& part : normal.GetParts()) for (const auto& vertex : part.vertices)
        {
            const auto before = normal.GetMatrices()[vertex.node] * vertex.position;
            const auto after = enlarged.GetMatrices()[vertex.node] * vertex.position;
            const auto expected = upperBody[vertex.node] ? before + offset : before * 1.40f;
            assert((after - expected).Length() < 0.002f);
        }
    }

    constexpr float pi = 3.14159265358979323846f;
    // 水平旋回だけでなく、右上・左下・真上・真下の照準でも脚を巻き込まないか調べる。
    const std::pair<float, float> aimAngles[] = {
        {-pi, 0.0f}, {-pi / 2, 0.0f}, {pi / 2, 0.0f}, {pi, 0.0f}, {2 * pi, 0.0f},
        {pi / 4, pi / 6}, {-pi / 3, -pi / 4}, {pi, pi / 2}, {0.0f, -pi / 2}
    };
    for (auto motion : {RobotMotion::Idle, RobotMotion::Forward, RobotMotion::Backward, RobotMotion::Left, RobotMotion::Right})
    {
        for (const auto& [yaw, pitch] : aimAngles)
        {
            auto straight = original, turned = original;
            straight.Update(motion, 0.25f);
            turned.Update(motion, 0.25f, 1.0f, yaw, pitch);
            const aiVector3D pivot = straight.GetMatrices()[top] * aiVector3D(0, 0, 0);
            const aiVector3D turnedPivot = turned.GetMatrices()[top] * aiVector3D(0, 0, 0);
            assert((turnedPivot - pivot).Length() < 0.001f); // 関節の位置はずれない。

            size_t upperVertices = 0, lowerVertices = 0;
            for (const auto& part : original.GetParts())
            {
                for (const auto& vertex : part.vertices)
                {
                    const auto before = straight.GetMatrices()[vertex.node] * vertex.position;
                    const auto after = turned.GetMatrices()[vertex.node] * vertex.position;
                    if (upperBody[vertex.node])
                    {
                        const auto relative = before - pivot;
                        const float pitchedY = relative.y * std::cos(pitch) - relative.z * std::sin(pitch);
                        const float pitchedZ = relative.y * std::sin(pitch) + relative.z * std::cos(pitch);
                        const aiVector3D expected = pivot + aiVector3D(
                            relative.x * std::cos(yaw) + pitchedZ * std::sin(yaw),
                            pitchedY,
                            -relative.x * std::sin(yaw) + pitchedZ * std::cos(yaw));
                        assert((after - expected).Length() < 0.005f);
                        ++upperVertices;
                    }
                    else
                    {
                        assert((after - before).Length() == 0.0f); // 脚は一切旋回しない。
                        ++lowerVertices;
                    }
                }
            }
            assert(upperVertices > 100 && lowerVertices > 100);
            const auto paused = turned.GetMatrices();
            turned.Update(motion, 0, 1.0f, yaw + pi / 2, pitch + pi / 4);
            assert(Difference(paused, turned.GetMatrices()) == 0);
        }

        // 旋回を続けても回転が累積せず、1周したら同時刻の元の姿勢へ戻る。
        auto straight = original, turning = original;
        for (int frame = 1; frame <= 120; ++frame)
        {
            straight.Update(motion, 1.0f / 60);
            const float yaw = 2 * pi * frame / 120;
            turning.Update(motion, 1.0f / 60, 1.0f, yaw, 0.4f * std::sin(yaw));
        }
        assert(Difference(straight.GetMatrices(), turning.GetMatrices()) < 0.01f);
    }

    // カメラが横や後ろを向いていても、固定された脚に対する移動方向を選ぶ。
    assert(SelectRobotWalkMotion(0, 0, 0) == RobotMotion::Idle);
    assert(SelectRobotWalkMotion(0, 1, 0) == RobotMotion::Forward);
    assert(SelectRobotWalkMotion(0, -1, 0) == RobotMotion::Backward);
    assert(SelectRobotWalkMotion(1, 0, 0) == RobotMotion::Right);
    assert(SelectRobotWalkMotion(-1, 0, 0) == RobotMotion::Left);
    assert(SelectRobotWalkMotion(1, 0, pi / 2) == RobotMotion::Forward);
    assert(SelectRobotWalkMotion(-1, 0, pi / 2) == RobotMotion::Backward);
    assert(SelectRobotWalkMotion(0, -1, pi / 2) == RobotMotion::Right);
    assert(SelectRobotWalkMotion(0, 1, pi / 2) == RobotMotion::Left);
    assert(SelectRobotWalkMotion(1, 0.2f, 0) == RobotMotion::Right);
}

int main()
{
    RobotAnimationRig original;
    std::string error;
    if (!original.Load("DX22_01_plane/assets/model/Player", error))
    {
        std::cerr << error << '\n';
        return 1;
    }
    size_t triangles = 0;
    aiVector3D minimum(1e9f), maximum(-1e9f);
    for (const auto& part : original.GetParts())
    {
        triangles += part.vertices.size() / 3;
        for (const auto& v : part.vertices)
        {
            auto p = original.GetMatrices()[v.node] * v.position;
            for (int c = 0; c < 3; ++c)
            {
                minimum[c] = (std::min)(minimum[c], p[c]);
                maximum[c] = (std::max)(maximum[c], p[c]);
            }
        }
    }
    assert(triangles == 25996); // Only the four LOD0 meshes; never double-render LOD1.
    std::cout << "parts=" << original.GetParts().size() << " triangles=" << triangles
              << " bounds=" << minimum.x << ',' << minimum.y << ',' << minimum.z
              << " to " << maximum.x << ',' << maximum.y << ',' << maximum.z << '\n';
    assert(maximum.x - minimum.x < 2000 && maximum.y - minimum.y < 1500);
    assert(minimum.y > -50 && minimum.y < 100); // Feet stay near the ground, no hierarchy explosion.

    for (auto motion : {RobotMotion::Idle, RobotMotion::Forward, RobotMotion::Backward, RobotMotion::Left, RobotMotion::Right})
    {
        auto rig = original;
        rig.Update(motion, 0.2f);
        auto a = rig.GetMatrices();
        rig.Update(motion, 0.3f);
        assert(Difference(a, rig.GetMatrices()) > 0.01f); // Real bone movement, not just changing labels.
        a = rig.GetMatrices();
        rig.Update(motion, 0);
        assert(Difference(a, rig.GetMatrices()) == 0); // Pause freezes the pose.

        auto slow = original, fast = original;
        for (int i = 0; i < 30; ++i) slow.Update(motion, 1.0f / 30, 1.0f, 0.7f, 0.4f);
        for (int i = 0; i < 120; ++i) fast.Update(motion, 1.0f / 120, 1.0f, 0.7f, 0.4f);
        assert(Difference(slow.GetMatrices(), fast.GetMatrices()) < 0.05f);
        // Repeated loops cannot accumulate root-motion translation.
        for (int i = 0; i < 1800; ++i) rig.Update(motion, 1.0f / 60);
        for (const auto& matrix : rig.GetMatrices())
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c) assert(std::isfinite(matrix[r][c]));
    }
    CheckUpperBodyAim(original);
    std::cout << "Robot animation asset, LOD, motion, frame-rate and upper-body yaw/pitch checks passed.\n";
}
