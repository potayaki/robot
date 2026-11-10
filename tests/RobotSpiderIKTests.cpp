#include "../DX22_01_plane/RobotSpiderIK.h"
#include "../DX22_01_plane/RobotDebris.h"
#include <cassert>
#include <cmath>
#include <iostream>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

aiMatrix4x4 World(aiVector3D position, float yaw = 0)
{
    return aiMatrix4x4(aiVector3D(0.025f), aiQuaternion(aiVector3D(0, 1, 0), yaw), position);
}

int main()
{
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    RobotAnimationRig original;
    std::string error;
    assert(original.Load("DX22_01_plane/assets/model/Player", error));
    const int joints[][4] = {{3,4,5,6}, {9,10,11,12}, {15,16,17,18},
        {27,28,29,30}, {33,34,35,36}, {39,40,41,42}};
    const auto rest = original.GetMatrices();
    for (int fps : {30, 60, 144})
    {
        auto rig = original;
        RobotSpiderIK ik;
        assert(ik.Init(rig));
        const float dt = 1.0f / fps;
        aiVector3D position(0, -5, 0);
        float maxError = 0, maxSlide = 0, maxTilt = 0, maxLag = 0;
        int lifted = 0;
        for (int frame = 0; frame < fps * 10; ++frame)
        {
            const float time = frame * dt;
            aiVector3D velocity;
            if (time >= 1 && time < 3) velocity = aiVector3D(0, 0, 60);
            else if (time >= 3 && time < 4) velocity = aiVector3D(60, 0, 0);
            else if (time >= 4 && time < 5) velocity = aiVector3D(-42, 0, -42);
            else if (time >= 5 && time < 6) velocity = aiVector3D(0, 0, -96);
            position += velocity * dt;
            const float yaw = time > 7 && time < 8 ? (time - 7) : (time >= 8 ? 1.0f : 0.0f);
            const auto previousFeet = ik.GetFeet();
            const auto previousContacts = ik.GetSolvedContacts();
            rig.Update(RobotMotion::Idle, dt, 1, 0.6f, 0.25f);
            ik.Update(rig, dt, World(position, yaw), -5, true);
            int support = 0;
            for (size_t i = 0; i < 6; ++i)
            {
                const auto& foot = ik.GetFeet()[i];
                support += !foot.swinging;
                if (foot.swinging && foot.position.y > -4.9f) ++lifted;
                maxError = (std::max)(maxError, (ik.GetSolvedContacts()[i] - foot.position).Length());
                if (frame > 0 && !foot.swinging && !previousFeet[i].swinging &&
                    foot.stepNumber == previousFeet[i].stepNumber)
                    maxSlide = (std::max)(maxSlide, (ik.GetSolvedContacts()[i] - previousContacts[i]).Length());
                for (int j = 0; j < 3; ++j)
                {
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
            for (const auto& m : rig.GetMatrices()) for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c) assert(std::isfinite(m[r][c]));
        }
        std::cout << fps << "fps contact error=" << maxError << " planted slide=" << maxSlide
            << " tilt=" << maxTilt << " lag=" << maxLag << std::endl;
        assert(maxError < 0.15f && maxSlide < 0.15f);
        assert(lifted > fps && maxTilt > 0.02f && maxLag > 0.2f);
        assert(ik.GetBodyTilt().Length() < 0.002f);
        const auto frozen = ik.GetSolvedContacts();
        ik.Update(rig, 0, World(position + aiVector3D(5, 0, 0)), -5, true);
        for (size_t i = 0; i < 6; ++i) assert((frozen[i] - ik.GetSolvedContacts()[i]).Length() == 0);
        // Jump/release anchors, land, then teleport without carrying old planted targets.
        for (int f = 0; f < fps; ++f)
        {
            rig.Update(RobotMotion::Idle, dt);
            ik.Update(rig, dt, World(position + aiVector3D(0, 10, 0)), -5, false);
        }
        ik.Update(rig, dt, World(position), -5, true);
        for (const auto& foot : ik.GetFeet()) assert(std::abs(foot.position.y + 5) < 0.001f);
        ik.Update(rig, dt, World(position + aiVector3D(500, 0, 0)), -5, true);
        for (size_t i = 0; i < 6; ++i)
            assert((ik.GetSolvedContacts()[i] - ik.GetFeet()[i].position).Length() < 0.15f);

        // Aiming changes only the upper branch, even while the body is sprung and feet are planted.
        auto aimedRig = rig;
        auto aimedIK = ik;
        const auto lastWorld = World(position + aiVector3D(500, 0, 0));
        rig.Update(RobotMotion::Idle, dt, 1, 0, 0);
        aimedRig.Update(RobotMotion::Idle, dt, 1, 1.0f, 0.4f);
        ik.Update(rig, dt, lastWorld, -5, true);
        aimedIK.Update(aimedRig, dt, lastWorld, -5, true);
        for (const auto& leg : joints) for (int joint : leg)
            for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c)
                assert(std::abs(rig.GetMatrices()[joint][r][c] - aimedRig.GetMatrices()[joint][r][c]) < 0.0001f);

        RobotDebris debris;
        debris.Start(aimedRig, lastWorld, -5);
        assert(debris.IsActive());
        // Starting the death effect must preserve every vertex of the current IK pose.
        double before[3]{}, after[3]{};
        size_t count = 0;
        for (const auto& part : aimedRig.GetParts()) for (const auto& vertex : part.vertices)
        {
            const auto p = lastWorld * aimedRig.GetMatrices()[vertex.node] * vertex.position;
            for (int axis = 0; axis < 3; ++axis) before[axis] += p[axis];
            ++count;
        }
        for (const auto& vertex : debris.GetVertices())
        {
            const auto p = debris.GetMatrices()[vertex.node] * vertex.position;
            for (int axis = 0; axis < 3; ++axis) after[axis] += p[axis];
        }
        assert(count == debris.GetVertices().size());
        for (int axis = 0; axis < 3; ++axis) assert(std::abs(before[axis] - after[axis]) / count < 0.001);
    }
    std::cout << "Six-foot IK, support, foot locking, weight, pause, landing and teleport checks passed.\n";
}
