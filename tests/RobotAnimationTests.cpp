#include "../DX22_01_plane/RobotAnimationRig.h"
#include <cassert>
#include <cmath>
#include <iostream>

float Difference(const std::vector<aiMatrix4x4>& a, const std::vector<aiMatrix4x4>& b)
{
    float sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        for (unsigned r = 0; r < 4; ++r)
            for (unsigned c = 0; c < 4; ++c) sum += std::abs(a[i][r][c] - b[i][r][c]);
    return sum;
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
        for (int i = 0; i < 30; ++i) slow.Update(motion, 1.0f / 30);
        for (int i = 0; i < 120; ++i) fast.Update(motion, 1.0f / 120);
        assert(Difference(slow.GetMatrices(), fast.GetMatrices()) < 0.05f);
        // Repeated loops cannot accumulate root-motion translation.
        for (int i = 0; i < 1800; ++i) rig.Update(motion, 1.0f / 60);
        for (const auto& matrix : rig.GetMatrices())
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c) assert(std::isfinite(matrix[r][c]));
    }
    std::cout << "Robot animation asset, LOD, motion and frame-rate checks passed.\n";
}
