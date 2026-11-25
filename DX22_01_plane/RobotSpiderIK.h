/// @file RobotSpiderIK.h
/// @brief 6本脚の接地点を保ちながら歩行と胴体の揺れを作るIK。

#pragma once

#include "RobotAnimationRig.h"
#include <array>
#include <functional>

// 6本脚の接地点をワールド座標で保持し、IK（逆運動学）で関節を曲げる。
// 足運びは「右後・右前・左中」と「右中・左後・左前」の3本ずつを交互に動かす。
// 距離は特記がなければゲーム内のワールド単位、時間は秒、角度はラジアン。
class RobotSpiderIK {
public:
    using HeightAt = std::function<float(float, float)>;

    /// 素材に含まれる脚の総数。
    static constexpr int LegCount = 6;

    // ImGuiのスライダーと内部の入力チェックで同じ範囲を使う。
    // 調整範囲を変えるときはここだけを変更する。脚の届く範囲は姿勢によって変わる。
    struct TuningLimits {
        /// 歩幅の下限。
        static constexpr float MinStrideLength = 8.0f;
        /// 大股を試すための歩幅の上限。
        static constexpr float MaxStrideLength = 32.0f;
        /// 元の接地幅に対する最小倍率。
        static constexpr float MinStanceWidth = 0.65f;
        /// 元の接地幅に対する最大倍率。
        static constexpr float MaxStanceWidth = 1.05f;
        /// 腰を下げられる量。
        static constexpr float MinBodyHeight = -0.5f;
        /// 腰を上げられる量。
        static constexpr float MaxBodyHeight = 2.5f;
        /// 足を持ち上げる高さの下限。
        static constexpr float MinStepHeight = 0.2f;
        /// 足を持ち上げる高さの上限。
        static constexpr float MaxStepHeight = 2.0f;
        /// 胴体を接地点から前後・左右にずらせる距離（ワールド単位）。
        static constexpr float MaxBodyShift = 2.0f;
        /// 胴体の手動回転の各軸の上限（ラジアン）。
        static constexpr float MaxBodyAngle = 0.25f;
        /// 足先を基準位置から前後・左右にずらせる距離（ワールド単位）。
        static constexpr float MaxFootShift = 3.0f;
        /// 個別の足先を地表から持ち上げられる高さ（ワールド単位）。
        static constexpr float MaxFootLift = 4.0f;
        /// 足先の到達誤差がこれを超えたら表示。
        static constexpr float ContactWarningDistance = 0.3f;
    };

    struct Settings {
        /// 足が基準位置からこの距離だけ遅れたら踏み出す。
        float stepDistance = 4.0f;
        /// 同じ足の着地から次の着地までの希望距離。
        float strideLength = 20.0f;
        /// 通常の振り出し時間の下限。歩き始め・切り返し時はさらに短縮する。
        float minStepSeconds = 0.09f;
        /// 最長時間。通常速度60で歩幅32に必要な約0.267秒を許可する。
        float maxStepSeconds = 0.30f;
        /// 振り出しの頂点で足を持ち上げる高さ。
        float stepHeight = 1.0f;
        /// 空中の足を外へ回す距離。腰の真下へ脚が折れ込むのを防ぐ。
        float swingOutward = 0.9f;
        /// 足を接地させたまま腰を上下する調整量。
        float bodyHeight = 0.6f;
        /// 足を固定したまま胴体を機体の右へずらす距離（ワールド単位）。
        float bodyRight = 0.0f;
        /// 足を固定したまま胴体を機体の前へずらす距離（ワールド単位）。
        float bodyForward = 0.0f;
        /// 胴体の追加回転。モデル軸のXがピッチ、Yがヨー、Zがロール（ラジアン）。
        aiVector3D bodyRotation;
        /// 各足の基準接地点からの距離。Xは機体の右、Yは地表からの高さ、Zは前（ワールド単位）。
        std::array<aiVector3D, LegCount> footOffsets{};
        /// 腰から足先までの水平距離の倍率。1.0で素材の元の幅。
        float stanceWidth = 0.90f;
        /// 胴体の位置が目標へ戻る速さ（Hz）。大きいほど素早く戻る。
        float bodyFrequency = 2.8f;
        /// 胴体の位置の揺れを抑える強さ。1.0付近で往復が少なくなる。
        float bodyDamping = 0.72f;
        /// 胴体の傾きが目標へ戻る速さ（Hz）。
        float tiltFrequency = 2.2f;
        /// 胴体の傾きの揺れを抑える強さ。
        float tiltDamping = 0.68f;
    };

    struct Foot {
        /// 現在の足先の接地点（ワールド座標）。接地中は固定する。
        aiVector3D position;
        /// 一歩を開始した位置。停止して着地し直すときにも更新する。
        aiVector3D start;
        /// 着地予定位置（ワールド座標）。空中にいる間だけ進行方向へ追従する。
        aiVector3D target;
        /// 足先の水平速度。着地先が変わっても滑らかにつなぐために保持する。
        aiVector3D velocity;
        /// 腰から外側へ向かう水平の単位ベクトル。
        aiVector3D outward;
        /// この一歩の頂点で足を外側へ回す距離（ワールド単位）。
        float arcWidth = 0.0f;
        /// 外側へ回り込む弧の現在のずれ。次の補間の前に取り除く。
        aiVector3D arcOffset;
        /// trueなら足を持ち上げて移動中、falseなら接地中。
        bool swinging = false;
        /// 真逆へ切り返した遊脚は、直前に離れた接地点まで軌道を逆にたどる。
        bool retracing = false;
        /// 一歩の進行率。0で開始、1で着地。
        float progress = 0.0f;
        /// 一歩にかける秒数。振り出し開始時に必ず設定する。
        float duration = 0.0f;
        /// 今の一歩の持ち上げ量。操作を離した後の着地では0にする。
        float liftHeight = 0.0f;
        /// 踏み出した回数。デバッグや接地の検証に使う。
        unsigned stepNumber = 0;
    };

    /// 現在の調整値。初期化後の変更はApplySettings経由で姿勢にも反映する。
    Settings m_settings;

    /// 使用前の状態と必要な資源を初期化する。
    bool Init(const RobotAnimationRig& rig);

    // アニメーションの時間を進めず、設定と表示姿勢を更新する。一時停止中にも使用できる。
    void ApplySettings(RobotAnimationRig& rig, const Settings& requested, const HeightAt& heightAt = {});
    void Reset(); // 接地の追跡と胴体の揺れを解除する。設定値と素材の基準姿勢は残す。

    // worldはモデルからワールドへの変換、groundYは地面の高さ、groundedは本体の接地状態。
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, float groundY, bool grounded,
                const HeightAt& terrain = {});

    const std::array<Foot, LegCount>& GetFeet() const {
        return m_feet;
    }

    const aiVector3D& GetBodyOffset() const {
        return m_body.value;
    }

    const aiVector3D& GetBodyTilt() const {
        return m_tilt.value;
    }

    const std::array<aiVector3D, LegCount>& GetSolvedContacts() const {
        return m_solvedContacts;
    }

    float GetContactError() const; // 目標の接地点と実際の足先の距離のうち、6本で最大の値。

private:
    // joints配列を数字で参照せず、どの関節を扱うかを名前で示す。
    enum JointIndex {
        Coxa, // 胴体との付け根。脚全体を左右へ向ける関節。
        Hip, // 脚の上側を曲げる関節。
        Knee, // 膝。HipとAnkleの間にある関節。
        Ankle, // 足首。足裏の向きを水平に保つ関節。
        JointCount
    };

    struct Spring {
        /// 現在の変位、または傾き。
        aiVector3D value;
        /// 変位、または傾きが変わる速度（毎秒）。
        aiVector3D velocity;

        /// 経過時間と入力に合わせて状態を更新する。
        void Update(aiVector3D target, float dt, float frequency, float damping);
    };

    struct Leg {
        /// Coxa→Hip→Knee→Ankleの骨番号。
        std::array<int, JointCount> joints{};
        /// 足の開きを反映した基準接地点（モデル座標）。
        aiVector3D home;
        /// 素材から求めた元の接地点。倍率の累積を防ぐため変更しない。
        aiVector3D restContact;
        /// 足裏から足首までのずれ（モデル座標）。
        aiVector3D ankleOffset;
        /// 足首のローカル座標で表した足裏の点。
        aiVector3D localContact;
    };

    /// 各脚の骨情報と基準姿勢。右後・右中・右前・左後・左中・左前。
    std::array<Leg, LegCount> m_legs;
    /// 各足の接地状態と振り出しの進行状況。
    std::array<Foot, LegCount> m_feet;
    /// IK計算後の実際の足先（ワールド座標）。
    std::array<aiVector3D, LegCount> m_solvedContacts;
    /// 下半身の拡大を反映済みの基準骨行列（モデル座標）。
    std::vector<aiMatrix4x4> m_rest;
    /// 踏み切る直前の実際の足先（モデル座標）。歩行の途中でも同じ姿勢から開き始める。
    std::array<aiVector3D, LegCount> m_takeoffFeet{};
    /// 今回のジャンプで観測した上昇速度の最大値（ワールド単位/秒）。
    float m_launchSpeed = 0.0f;
    /// 本体のワールドY方向の速度。頂点を境に符号が変わる（ワールド単位/秒）。
    float m_verticalSpeed = 0.0f;
    /// 本体のモデル原点から地表までの高さ（ワールド単位）。
    float m_groundClearance = 0.0f;
    /// 胴体の揺れと移動への遅れ（ワールド座標の変位）。
    Spring m_body;
    /// 胴体の前後・左右への傾き（モデル座標の角度）。
    Spring m_tilt;
    /// 最後に接地した面のX・Z方向の高さ勾配（ワールド座標）。着地姿勢への補間の起点。
    aiVector3D m_supportGradient;
    /// 前回の細分更新時の本体位置。移動速度の計算に使う。
    aiVector3D m_previousPosition;
    /// 揺れ用にならした本体の移動速度（ワールド座標）。
    aiVector3D m_filteredVelocity;
    /// 方向転換を検出するための、ならす前の前回速度。
    aiVector3D m_previousVelocity;
    /// 必要な関節と足裏の情報を取得できたか。
    bool m_initialized = false;
    /// 接地点の追跡が始まっているか。
    bool m_tracking = false;
    /// 前回の接地状態。空中からの着地を検出する。
    bool m_grounded = false;
    /// 前回移動していたか。歩き始めと停止の判定に使う。
    bool m_wasMoving = false;
    /// 次に動かす3本組。偶数番号の足が0、奇数番号の足が1。
    int m_nextTripod = 0;
    /// 急な方向転換後、短い一歩を行う残りの3本組の数。
    int m_turnSteps = 0;
    /// 前フレームの変換。細分更新の補間と停止中の調整に使う。
    aiMatrix4x4 m_previousWorld;

    aiVector3D GetHome(const Leg& leg) const;
    /// 歩行と旋回で目指すワールド座標の足位置。高さは着地点の地表に合わせて決める。
    aiVector3D GetWorldHome(size_t index, const aiMatrix4x4& world) const;
    /// 指定地点の地表に、個別の足の持ち上げ量を加えたワールドY座標。
    float GetFootHeight(size_t index, const aiVector3D& point, const HeightAt& heightAt) const;
    void ResetContacts(const aiMatrix4x4& world, const HeightAt& heightAt, bool grounded);
    bool UpdateSwingingFeet(float dt, const aiMatrix4x4& world, const HeightAt& heightAt, bool moving,
                            const aiVector3D& velocity, float stepSeconds);
    void StartWalk(const aiMatrix4x4& world, const HeightAt& heightAt, const aiVector3D& velocity,
                   float stepSeconds, float dt);
    void BeginTripodStep(int group, const aiMatrix4x4& world, const HeightAt& heightAt,
                         const aiVector3D& velocity, float duration, float stepSeconds);
    void UpdateBodyMotion(float dt, const aiMatrix4x4& world, bool grounded, float speed,
                          const aiVector3D& acceleration);
    void Step(RobotAnimationRig& rig, float dt, const aiMatrix4x4& world, const HeightAt& heightAt,
              bool grounded, float groundY);
    void Solve(RobotAnimationRig& rig, const aiMatrix4x4& world, const HeightAt& heightAt);
    void PoseAirborneFeet(const aiMatrix4x4& world, const aiMatrix4x4& body, const HeightAt& heightAt,
                          float tuck, float approach);
    void SolveLegs(RobotAnimationRig& rig, const aiMatrix4x4& inverseWorld);
    void TransformBranch(RobotAnimationRig& rig, int root, const aiMatrix4x4& transform);
};
