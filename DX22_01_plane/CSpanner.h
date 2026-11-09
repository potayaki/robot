#pragma once
#include "TestModel.h"

// Enemy drop: repairs the robot when collected.
class CSpanner : public Object
{
public:
    // Drop chance for every enemy defeat, in percent. 0 = never, 100 = always.
    inline static int m_spawnChancePercent = 15;
    int m_healPercent = 25; // Percentage of maximum HP restored.
    float m_baseScale = 5.0f; // About 38 units tall before pulsing.
    float m_pulseAmount = 0.20f;
    float m_rotationSpeed = 2.0f; // Radians per second.

    ~CSpanner() override { Uninit(); }
    void Init() override;
    void Update() override;
    void Draw(Camera* camera) override;
    void Uninit() override;
    static void LoadResource();
    static void UnloadResource();
    Collision::Sphere GetCollisionSphere() override { return {m_Position, m_pickupRadius}; }
private:
    inline static std::unique_ptr<TestModel> s_model;
    inline static int s_count = 0;
    int m_maxCount = 15;
    float m_pickupRadius = 14.0f;
    float m_elapsed = 0;
    bool m_counted = false;
};
