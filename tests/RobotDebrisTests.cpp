#include "../DX22_01_plane/RobotDebris.h"
#include <cassert>
#include <cmath>
#include <iostream>

float MatrixDifference(const std::vector<aiMatrix4x4>& a, const std::vector<aiMatrix4x4>& b)
{
    assert(a.size() == b.size());
    float difference = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
        for (unsigned row = 0; row < 4; ++row)
            for (unsigned column = 0; column < 4; ++column)
                difference += std::abs(a[i][row][column] - b[i][row][column]);
    return difference;
}

struct GeometryStats
{
    double x = 0, y = 0, z = 0, area = 0;
    size_t vertices = 0;
    void AddTriangle(const aiVector3D& a, const aiVector3D& b, const aiVector3D& c)
    {
        for (const auto& point : {a, b, c})
        {
            x += point.x;
            y += point.y;
            z += point.z;
            ++vertices;
        }
        area += ((b - a) ^ (c - a)).Length() * 0.5;
    }
};

int main()
{
    RobotAnimationRig rig;
    std::string error;
    assert(rig.Load("DX22_01_plane/assets/model/Player", error));
    // 歩行途中に上半身を右上へ向けた状態で死亡させ、待機姿勢へ戻らないことを調べる。
    rig.Update(RobotMotion::Left, 0.37f, 1.0f, 0.8f, 0.3f);
    const aiMatrix4x4 world(aiVector3D(0.025f, 0.025f, 0.025f),
        aiQuaternion(aiVector3D(0, 1, 0), 1.2f), aiVector3D(120, 30, -400));
    constexpr float groundY = -5.0f;
    RobotDebris debris;
    debris.Start(rig, world, groundY);
    assert(debris.IsActive());
    assert(debris.GetMatrices().size() >= 12 && debris.GetMatrices().size() <= 128);

    GeometryStats before, after;
    for (const auto& part : rig.GetParts())
    {
        for (size_t i = 0; i < part.vertices.size(); i += 3)
        {
            aiVector3D points[3];
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = part.vertices[i + corner];
                points[corner] = world * rig.GetMatrices()[vertex.node] * vertex.position;
            }
            before.AddTriangle(points[0], points[1], points[2]);
        }
    }
    const auto& vertices = debris.GetVertices();
    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        const auto& a = vertices[i];
        const auto& b = vertices[i + 1];
        const auto& c = vertices[i + 2];
        assert(a.node == b.node && b.node == c.node); // 関節をまたぐ面も、別々の破片へ引き裂かない。
        const auto& matrix = debris.GetMatrices()[a.node];
        after.AddTriangle(matrix * a.position, matrix * b.position, matrix * c.position);
        assert(std::abs(a.normal.Length() - 1.0f) < 0.001f);
    }
    assert(before.vertices == after.vertices && after.vertices == 25996 * 3);
    assert(std::abs(before.x - after.x) < 0.2);
    assert(std::abs(before.y - after.y) < 0.2);
    assert(std::abs(before.z - after.z) < 0.2);
    assert(std::abs(before.area - after.area) < 0.1); // 分解した瞬間に形状・面積が変わらない。

    const auto initial = debris.GetMatrices();
    debris.Update(0.25f);
    assert(MatrixDifference(initial, debris.GetMatrices()) > 10.0f);
    const auto moved = debris.GetMatrices();
    debris.Update(0);
    debris.Start(rig, world, groundY);
    assert(MatrixDifference(moved, debris.GetMatrices()) == 0); // 停止中やHP0の再通知では再発生しない。

    // 破片が回転しても、三角形の辺の長さを保つ（骨間がゴムのように伸びない）。
    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        const auto& matrix = moved[vertices[i].node];
        const auto a = matrix * vertices[i].position;
        const auto b = matrix * vertices[i + 1].position;
        assert(std::abs((a - b).Length() - (vertices[i].position - vertices[i + 1].position).Length()) < 0.001f);
    }

    RobotDebris slow, fast;
    slow.Start(rig, world, groundY);
    fast.Start(rig, world, groundY);
    for (int i = 0; i < 30 * 8; ++i) slow.Update(1.0f / 30);
    for (int i = 0; i < 120 * 8; ++i) fast.Update(1.0f / 120);
    assert(MatrixDifference(slow.GetMatrices(), fast.GetMatrices()) < 0.01f);
    for (const auto& vertex : slow.GetVertices())
    {
        const auto point = slow.GetMatrices()[vertex.node] * vertex.position;
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
        assert(point.y >= groundY - 0.001f); // すべての破片が床より上にある。
    }
    const auto settled = slow.GetMatrices();
    slow.Update(1.0f);
    assert(MatrixDifference(settled, slow.GetMatrices()) == 0); // 着地した残骸は落ち着いて停止する。

    debris.Reset();
    assert(!debris.IsActive() && debris.GetVertices().empty() && debris.GetMatrices().empty());
    debris.Start(rig, world, groundY);
    assert(MatrixDifference(initial, debris.GetMatrices()) == 0); // Debugで復帰した後の再テスト。
    std::cout << "Robot debris: " << initial.size() << " fragments, " << after.vertices / 3
              << " triangles; pose, rigid faces, one-shot, pause, ground, FPS and reset checks passed.\n";
}
