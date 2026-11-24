/// @file RobotAnimationRig.h
/// @brief ロボットの素材読込、歩行の補間、上半身の照準回転。

#pragma once
#include <assimp/types.h>
#include <array>
#include <string>
#include <vector>

enum class RobotMotion {
    Idle,
    Forward,
    Backward,
    Left,
    Right,
    Count
};

// ワールド上の移動量を、固定された脚側の正面・右方向に変換して歩行を選ぶ。
RobotMotion SelectRobotWalkMotion(float moveX, float moveZ, float lowerBodyYaw);

// このメカは全頂点が1本の骨に固定されている。各頂点の骨番号をGPUに渡すことで、
// 毎フレームCPUで全頂点を変形せずに骨格アニメーションを再生できる。
// GPUに依存しないため、実際のFBXを使った再生・FPS・切り替えの検証にも使用する。
class RobotAnimationRig {
    friend class RobotSpiderIK;

public:
    struct GunMount {
        /// 片側の上下の銃身に固定された骨番号。
        int top = -1;
        int bottom = -1;
        /// 銃口が向く骨ローカルZ軸の符号。左は+Z、右は-Z。
        float forwardSign = 1.0f;
        /// 上側の骨姿勢からモデル座標の銃口と前向き単位ベクトルを求める。
        aiVector3D Muzzle(const aiMatrix4x4& topPose) const;
        aiVector3D Axis(const aiMatrix4x4& topPose) const;
    };

    struct Vertex {
        /// モデルまたはオブジェクトの位置。
        aiVector3D position;
        /// 頂点または面の法線方向。光の当たり方の計算に使用する。
        aiVector3D normal;
        /// テクスチャを参照するUV座標。
        aiVector3D uv;
        /// 頂点または骨が対応する骨格ノードの番号。
        unsigned node = 0;
    };

    struct Part {
        /// 三角形リスト。座標は対応する骨のローカル座標。
        std::vector<Vertex> vertices;
    };

    struct Pose {
        /// モデルの拡大縮小倍率。
        aiVector3D scale{1, 1, 1};
        /// モデルまたは部品の回転姿勢。
        aiQuaternion rotation;
        /// モデルまたはオブジェクトの位置。
        aiVector3D position;
    };

private:
    struct Node {
        /// 素材内で識別するための名前。
        std::string name;
        /// 親ノードの番号。-1は親を持たないルートを表す。
        int parent = -1;
        /// アニメーションを適用する前の基準姿勢。
        Pose bind;
        /// Top_M自身とその子（武器）。脚の枝は含まない。
        bool upperBody = false;
    };

    template <typename T> struct Key {
        /// アニメーションキーの時刻（素材の時間単位）。
        double time;
        /// このキーの時刻で使用する位置・回転・拡縮の値。
        T value;
    };

    struct Channel {
        /// 位置のキー、または形状を構成する座標の配列。
        std::vector<Key<aiVector3D>> positions;
        /// 拡縮のアニメーションキー。
        std::vector<Key<aiVector3D>> scales;
        /// 回転のアニメーションキー。
        std::vector<Key<aiQuaternion>> rotations;
    };

    struct Clip {
        /// アニメーション1周期の長さ（素材の時間単位）。
        double duration = 1;
        /// アニメーションの1秒あたりの時間単位数。
        double ticksPerSecond = 30;
        /// 骨ごとの位置・回転・拡縮のアニメーションキー。
        std::vector<Channel> channels;
    };

    /// 親子関係と基準姿勢を持つ骨格ノード一覧。
    std::vector<Node> m_nodes;
    /// モデルを構成する各パーツ。
    std::vector<Part> m_parts;
    /// 歩行方向ごとに読み込んだアニメーションの一覧。
    std::array<Clip, static_cast<size_t>(RobotMotion::Count)> m_clips;
    /// 各骨の現在のローカル姿勢。
    std::vector<Pose> m_pose;
    /// 動作を切り替える直前の姿勢。補間の始点に使用する。
    std::vector<Pose> m_blendFrom;
    /// 親子関係を合成した、モデル座標での各骨の変換行列。
    std::vector<aiMatrix4x4> m_globals;
    /// インデックス0は左の銃身、1は右の銃身。
    std::array<GunMount, 2> m_guns;
    /// 素材のルート変換を打ち消すための逆行列。
    aiMatrix4x4 m_inverseRoot;
    /// 現在再生している動作の種類。
    RobotMotion m_motion = RobotMotion::Idle;
    /// 再生中のクリップ上の時刻（素材の時間単位）。
    double m_time = 0;
    /// 動作切り替え後の補間に使用する経過時間（秒）。
    float m_blendTime = 0.15f;
    /// 照準に合わせて回転させる上半身のルート骨番号。
    int m_upperBodyNode = -1;
    /// 下半身を基準にした照準方向の差（ラジアン）。
    float m_upperBodyYaw = 0.0f;
    /// 上を狙うほど正になる仰角（ラジアン）。
    float m_upperBodyPitch = 0.0f;
    /// 上半身を保ったまま、腰と脚だけに適用する拡大率。
    float m_lowerBodyScale = 1.0f;
    void Evaluate();
    void ApplyLowerBodyScale();
    void ApplyUpperBodyAim();

public:
    /// 名前に対応する骨の番号。見つからない場合は -1。
    int FindNode(const std::string& name) const;

    const auto& GetGuns() const {
        return m_guns;
    }

    /// 照準・IKで確定した今の姿勢から、片側の上下の銃身をモデル単位で後退させる。
    void ApplyBarrelRecoil(size_t side, float distance);

    // 武器などに表示物を取り付ける際、現在の姿勢行列を名前で参照する。
    const aiMatrix4x4* FindNodeTransform(const std::string& name) const {
        const int node = FindNode(name);
        return node >= 0 ? &m_globals[node] : nullptr;
    }

    bool Load(const std::string& directory, std::string& error);
    // IKのInitより前に指定する。脚と腰だけ拡大し、上半身・武器の大きさは保つ。
    void SetLowerBodyScale(float scale);
    /// 経過秒数と照準から姿勢を更新する。holdPose中はクリップと補間を止め、上半身の照準を更新する。
    void Update(RobotMotion motion, float deltaSeconds, float playbackRate = 1.0f, float upperBodyYaw = 0.0f,
                float upperBodyPitch = 0.0f, bool holdPose = false);

    const std::vector<Part>& GetParts() const {
        return m_parts;
    }

    const std::vector<aiMatrix4x4>& GetMatrices() const {
        return m_globals;
    }

    RobotMotion GetMotion() const {
        return m_motion;
    }

    double GetTime() const {
        return m_time;
    }
};
