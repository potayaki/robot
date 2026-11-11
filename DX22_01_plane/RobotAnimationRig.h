#pragma once
#include <assimp/types.h>
#include <array>
#include <string>
#include <vector>

enum class RobotMotion { Idle, Forward, Backward, Left, Right, Count };

// ワールド上の移動量を、固定された脚側の正面・右方向に変換して歩行を選ぶ。
RobotMotion SelectRobotWalkMotion(float moveX, float moveZ, float lowerBodyYaw);

// このメカは全頂点が1本の骨に固定されている。各頂点の骨番号をGPUに渡すことで、
// 毎フレームCPUで全頂点を変形せずに骨格アニメーションを再生できる。
// GPUに依存しないため、実際のFBXを使った再生・FPS・切り替えの検証にも使用する。
class RobotAnimationRig
{
    friend class RobotSpiderIK;
public:
    struct Vertex { aiVector3D position, normal, uv; unsigned node = 0; };
    struct Part
    {
        std::vector<Vertex> vertices; // 三角形リスト。座標は対応する骨のローカル座標。
    };
    struct Pose
    {
        aiVector3D scale{1, 1, 1};
        aiQuaternion rotation;
        aiVector3D position;
    };

private:
    struct Node
    {
        std::string name;
        int parent = -1;
        Pose bind;
        bool upperBody = false; // Top_M自身とその子（武器）。脚の枝は含まない。
    };
    template<typename T> struct Key { double time; T value; };
    struct Channel
    {
        std::vector<Key<aiVector3D>> positions, scales;
        std::vector<Key<aiQuaternion>> rotations;
    };
    struct Clip
    {
        double duration = 1, ticksPerSecond = 30;
        std::vector<Channel> channels;
    };
    std::vector<Node> m_nodes;
    std::vector<Part> m_parts;
    std::array<Clip, static_cast<size_t>(RobotMotion::Count)> m_clips;
    std::vector<Pose> m_pose, m_blendFrom;
    std::vector<aiMatrix4x4> m_globals;
    aiMatrix4x4 m_inverseRoot;
    RobotMotion m_motion = RobotMotion::Idle;
    double m_time = 0;
    float m_blendTime = 0.15f;
    int m_upperBodyNode = -1;
    float m_upperBodyYaw = 0.0f; // 下半身を基準にした照準方向の差（ラジアン）。
    float m_upperBodyPitch = 0.0f; // 上を狙うほど正になる仰角（ラジアン）。
    float m_lowerBodyScale = 1.0f;
    int FindNode(const std::string& name) const;
    void Evaluate();
    void ApplyLowerBodyScale();
    void ApplyUpperBodyAim();

public:
    bool Load(const std::string& directory, std::string& error);
    // IKのInitより前に指定する。脚と腰だけ拡大し、上半身・武器の大きさは保つ。
    void SetLowerBodyScale(float scale);
    void Update(RobotMotion motion, float deltaSeconds, float playbackRate = 1.0f,
                float upperBodyYaw = 0.0f, float upperBodyPitch = 0.0f);
    const std::vector<Part>& GetParts() const { return m_parts; }
    const std::vector<aiMatrix4x4>& GetMatrices() const { return m_globals; }
    RobotMotion GetMotion() const { return m_motion; }
    double GetTime() const { return m_time; }
};
