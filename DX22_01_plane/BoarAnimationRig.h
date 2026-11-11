/// @file BoarAnimationRig.h
/// @brief Boarの骨格読込と接近・突進・死亡アニメーション。

#pragma once

#include <assimp/types.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

enum class BoarMotion {
    Idle,
    Run,
    Windup,
    Charge,
    Recover,
    Death
};

// Boar.glbの1本のタイムラインから、用途に合う区間を再生する。
// GPUに依存しないため、骨格・死亡ポーズ・再生速度をテストでも確認できる。
class BoarAnimationRig {
public:
    struct Vertex {
        /// モデルまたはオブジェクトの位置。
        aiVector3D position;
        /// 頂点または面の法線方向。光の当たり方の計算に使用する。
        aiVector3D normal;
        /// テクスチャを参照するUV座標。
        aiVector3D uv;
        /// 頂点の変形に使用する骨の情報。
        std::array<unsigned, 4> bones{};
        /// 各ボーンが頂点に与える影響度。スキニングの重みに使用する。
        std::array<float, 4> weights{};
    };

    struct Pose {
        /// モデルの拡大縮小倍率。
        aiVector3D scale{1, 1, 1};
        /// モデルまたはオブジェクトの位置。
        aiVector3D position;
        /// モデルまたは部品の回転姿勢。
        aiQuaternion rotation;
    };

    struct Bounds {
        /// 各軸の最小座標で表した境界の下限。
        aiVector3D min;
        /// 各軸の最大座標で表した境界の上限。
        aiVector3D max;
    };

    bool Load(const std::string& file, std::string& error);
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(BoarMotion motion, float deltaSeconds, float progress = 0.0f);
    bool DeathFinished() const;
    Bounds GetBounds() const;
    const std::vector<Vertex>& GetVertices() const;
    const std::vector<unsigned char>& GetAlbedo() const;

    const std::vector<aiMatrix4x4>& GetMatrices() const {
        return m_matrices;
    }

    double GetFrame() const {
        return m_frame;
    }

private:
    struct Data;
    /// 複数インスタンスで共有する読込済みの素材データ。
    std::shared_ptr<const Data> m_data;
    /// 各骨の現在のローカル姿勢。
    std::vector<Pose> m_pose;
    /// 動作を切り替える直前の姿勢。補間の始点に使用する。
    std::vector<Pose> m_blendFrom;
    /// 描画に使用する各骨、または各破片の変換行列。
    std::vector<aiMatrix4x4> m_matrices;
    /// 現在再生している動作の種類。
    BoarMotion m_motion = BoarMotion::Idle;
    /// 現在の動作を開始してからの経過秒数。
    double m_seconds = 0.0;
    /// 素材のタイムライン上で現在表示しているフレーム位置。
    double m_frame = 0.0;
    /// 動作の切り替えを滑らかにつなぐための経過秒数。
    float m_blendSeconds = 0.12f;

    void Evaluate();
};
