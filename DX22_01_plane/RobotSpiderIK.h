#pragma once
#include "RobotAnimationRig.h"
#include <array>

// 6本脚の接地点をワールド座標で保持し、IKで脚を曲げる。
// 歩幅・持ち上げ量はゲーム内の距離、踏み出し時間は秒で指定する。
class RobotSpiderIK
{
public:
    struct Settings
    {
        float stepDistance = 4.0f; // 接地点が基準位置から後ろに離れたら次の一歩を始める。
        float strideLength = 20.0f; // 同じ足の着地から次の着地までの目安。通常速度なら約0.33秒周期。
        float minStepSeconds = 0.09f; // 移動速度を強化した場合だけ短くする。通常歩行は約0.17秒。
        float maxStepSeconds = 0.24f;
        float stepHeight = 1.0f; // 大きく跳ね上げず、前後に広く足を運ぶ。
        float swingOutward = 1.2f; // 振り出す足を外へ回し、腰の真下で脚が折り畳まれるのを防ぐ。
        float bodyHeight = 0.4f; // 腰の上下調整量。低めの腰と外側の接地点で蜘蛛のような構えにする。
        float stanceWidth = 0.90f; // 腰から接地点までの開き。1.0で素材と同じ幅。大きいほど外側に置く。
        float bodyFrequency = 2.8f;
        float bodyDamping = 0.72f;
        float tiltFrequency = 2.2f;
        float tiltDamping = 0.68f;
    } settings;
    struct Foot
    {
        aiVector3D position, start, target, velocity;
        aiVector3D outward, arcOffset;
        bool swinging = false;
        float progress = 0;
        float duration = 0.15f;
        float liftHeight = 0;
        unsigned stepNumber = 0;
    };
    bool Init(const RobotAnimationRig& rig);
    // 実行中・一時停止中の調整用。アニメーションを進めず、設定と表示姿勢だけを更新する。
    void ApplySettings(RobotAnimationRig& rig, const Settings& requested);
    void Reset();
    void Update(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world,
        float groundY, bool grounded);
    const std::array<Foot, 6>& GetFeet() const { return m_feet; }
    const aiVector3D& GetBodyOffset() const { return m_body.value; }
    const aiVector3D& GetBodyTilt() const { return m_tilt.value; }
    const std::array<aiVector3D, 6>& GetSolvedContacts() const { return m_solvedContacts; }
    float GetContactError() const;
private:
    struct Spring
    {
        aiVector3D value, velocity;
        void Update(aiVector3D target, float dt, float frequency, float damping);
    };
    struct Leg
    {
        std::array<int, 4> joints{}; // Coxa, hip, knee, ankle; toe bones follow ankle.
        aiVector3D home, restContact, ankleOffset, localContact;
    };
    std::array<Leg, 6> m_legs;
    std::array<Foot, 6> m_feet;
    std::array<aiVector3D, 6> m_solvedContacts;
    std::vector<aiMatrix4x4> m_rest;
    Spring m_body, m_tilt;
    aiVector3D m_previousPosition, m_filteredVelocity;
    aiVector3D m_previousVelocity;
    bool m_initialized = false, m_tracking = false, m_grounded = false;
    bool m_wasMoving = false;
    int m_nextTripod = 0;
    int m_turnSteps = 0;
    aiMatrix4x4 m_previousWorld;
    aiVector3D GetHome(const Leg& leg) const;
    void Step(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, float groundY, bool grounded);
    void Solve(RobotAnimationRig& rig, const aiMatrix4x4& world);
    void TransformBranch(RobotAnimationRig& rig, int root, const aiMatrix4x4& transform);
};
