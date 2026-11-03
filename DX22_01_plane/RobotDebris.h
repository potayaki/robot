#pragma once
#include "RobotAnimationRig.h"

// 死亡時の表示専用。現在の姿勢を部品へ焼き付け、骨格から独立して飛ばす。
// GPUに依存しないため、分解前後の形状・接地・FPS差を実際のFBXで検証できる。
class RobotDebris
{
    struct Fragment
    {
        aiVector3D position;
        aiVector3D velocity;
        aiQuaternion rotation;
        aiVector3D spinAxis;
        float spinSpeed = 0.0f;
        float radius = 0.0f;
        size_t firstVertex = 0;
        size_t vertexCount = 0;
        bool resting = false;
    };

    std::vector<RobotAnimationRig::Vertex> m_vertices;
    std::vector<Fragment> m_fragments;
    std::vector<aiMatrix4x4> m_matrices;
    float m_groundY = 0.0f;
    double m_pendingSeconds = 0.0;

    void Step(float seconds);
    void UpdateMatrices();

public:
    void Start(const RobotAnimationRig& rig, const aiMatrix4x4& modelToWorld, float groundY);
    void Update(float deltaSeconds);
    void Reset();
    bool IsActive() const { return !m_fragments.empty(); }
    const std::vector<RobotAnimationRig::Vertex>& GetVertices() const { return m_vertices; }
    const std::vector<aiMatrix4x4>& GetMatrices() const { return m_matrices; }
};
