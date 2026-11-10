#pragma once
#include "RobotAnimationRig.h"
#include <array>

// World-space foot planting and procedural weight for the six-legged player.
// Units exposed here are game/world units (the FBX is scaled by 0.025).
class RobotSpiderIK
{
public:
    struct Settings
    {
        float stepDistance = 2.0f;
        float stepHeight = 1.8f;
        float bodyFrequency = 2.8f;
        float bodyDamping = 0.72f;
        float tiltFrequency = 2.2f;
        float tiltDamping = 0.68f;
    } settings;
    struct Foot
    {
        aiVector3D position, start, target;
        bool swinging = false;
        float progress = 0;
        unsigned stepNumber = 0;
    };
    bool Init(const RobotAnimationRig& rig);
    void Reset();
    void Update(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world,
        float groundY, bool grounded);
    const std::array<Foot, 6>& GetFeet() const { return m_feet; }
    const aiVector3D& GetBodyOffset() const { return m_body.value; }
    const aiVector3D& GetBodyTilt() const { return m_tilt.value; }
    const std::array<aiVector3D, 6>& GetSolvedContacts() const { return m_solvedContacts; }
private:
    struct Spring
    {
        aiVector3D value, velocity;
        void Update(aiVector3D target, float dt, float frequency, float damping);
    };
    struct Leg
    {
        std::array<int, 4> joints{}; // Coxa, hip, knee, ankle; toe bones follow ankle.
        aiVector3D home, ankleOffset, localContact;
    };
    std::array<Leg, 6> m_legs;
    std::array<Foot, 6> m_feet;
    std::array<aiVector3D, 6> m_solvedContacts;
    std::vector<aiMatrix4x4> m_rest;
    Spring m_body, m_tilt;
    aiVector3D m_previousPosition, m_filteredVelocity;
    bool m_initialized = false, m_tracking = false, m_grounded = false;
    int m_nextTripod = 0;
    float m_stepDuration = 0.15f;
    aiMatrix4x4 m_previousWorld;
    void Step(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, float groundY, bool grounded);
    void Solve(RobotAnimationRig& rig, const aiMatrix4x4& world);
    void TransformBranch(RobotAnimationRig& rig, int root, const aiMatrix4x4& transform);
};
