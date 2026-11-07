#pragma once

#include <assimp/types.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

enum class BoarMotion { Idle, Run, Windup, Charge, Recover, Death };

// Boar.glbの1本のタイムラインから、用途に合う区間を再生する。
// GPUに依存しないため、骨格・死亡ポーズ・再生速度をテストでも確認できる。
class BoarAnimationRig
{
public:
    struct Vertex
    {
        aiVector3D position, normal, uv;
        std::array<unsigned, 4> bones{};
        std::array<float, 4> weights{};
    };
    struct Pose
    {
        aiVector3D scale{1, 1, 1}, position;
        aiQuaternion rotation;
    };
    struct Bounds { aiVector3D min, max; };

    bool Load(const std::string& file, std::string& error);
    void Update(BoarMotion motion, float deltaSeconds, float progress = 0.0f);
    bool DeathFinished() const;
    Bounds GetBounds() const;
    const std::vector<Vertex>& GetVertices() const;
    const std::vector<unsigned char>& GetAlbedo() const;
    const std::vector<aiMatrix4x4>& GetMatrices() const { return m_matrices; }
    double GetFrame() const { return m_frame; }

private:
    struct Data;
    std::shared_ptr<const Data> m_data;
    std::vector<Pose> m_pose, m_blendFrom;
    std::vector<aiMatrix4x4> m_matrices;
    BoarMotion m_motion = BoarMotion::Idle;
    double m_seconds = 0.0, m_frame = 0.0;
    float m_blendSeconds = 0.12f;

    void Evaluate();
};
