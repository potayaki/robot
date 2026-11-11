/// @file RobotDebris.h
/// @brief 現在のロボット姿勢から作る破片と、落下・回転・着地。

#pragma once
#include "RobotAnimationRig.h"

// 死亡時の表示専用。現在の姿勢を部品へ焼き付け、骨格から独立して飛ばす。
// GPUに依存しないため、分解前後の形状・接地・FPS差を実際のFBXで検証できる。
class RobotDebris {
    struct Fragment {
        /// モデルまたはオブジェクトの位置。
        aiVector3D position;
        /// 位置を変化させる速度ベクトル（ワールド単位/秒）。
        aiVector3D velocity;
        /// モデルまたは部品の回転姿勢。
        aiQuaternion rotation;
        /// 破片が回転する軸の単位ベクトル。
        aiVector3D spinAxis;
        /// 破片の角速度（ラジアン/秒）。
        float spinSpeed = 0.0f;
        /// 地面との接触判定に使う破片の半径。
        float radius = 0.0f;
        /// この破片の頂点が始まる配列内の位置。
        size_t firstVertex = 0;
        /// 描画対象となる頂点の数。
        size_t vertexCount = 0;
        /// 破片が地面で静止したか。
        bool resting = false;
    };

    /// 描画に使用する頂点の配列。
    std::vector<RobotAnimationRig::Vertex> m_vertices;
    /// 分離した各部品の位置・速度・回転などの物理状態。
    std::vector<Fragment> m_fragments;
    /// 描画に使用する各骨、または各破片の変換行列。
    std::vector<aiMatrix4x4> m_matrices;
    /// 足元や破片の着地に使用する地面の高さ。
    float m_groundY = 0.0f;
    /// 破片の固定時間更新へまだ渡していない経過秒数。
    double m_pendingSeconds = 0.0;

    void Step(float seconds);
    void UpdateMatrices();

public:
    void Start(const RobotAnimationRig& rig, const aiMatrix4x4& modelToWorld, float groundY);
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(float deltaSeconds);
    void Reset();

    bool IsActive() const {
        return !m_fragments.empty();
    }

    const std::vector<RobotAnimationRig::Vertex>& GetVertices() const {
        return m_vertices;
    }

    const std::vector<aiMatrix4x4>& GetMatrices() const {
        return m_matrices;
    }
};
