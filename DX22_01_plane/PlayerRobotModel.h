/// @file PlayerRobotModel.h
/// @brief プレイヤーのロボット表示、足のIK、死亡時の分解表示。

#pragma once
#include "Object.h"
#include "RobotAnimationRig.h"
#include "RobotDebris.h"
#include "RobotSpiderIK.h"
#include "Texture.h"
#include "VertexBuffer.h"

// プレイヤー専用の表示。本体の移動・HP・当たり判定はCPlayerが引き続き管理する。
class PlayerRobotModel : public Object {
    /// playerRobotVS.hlslのBones配列と同じ上限。
    static constexpr size_t MaxBones = 128;

    struct SkinnedVertex {
        /// 位置・法線・色・UVをまとめた描画用頂点。
        VERTEX_3D vertex;
        /// この頂点が追従する骨番号。硬い機体なので1頂点につき1本。
        unsigned bone;
    };

    /// 素材の骨格と現在の姿勢。上半身の照準回転も管理する。
    RobotAnimationRig m_rig;
    /// 6本脚の足運び・接地・胴体の揺れ。
    RobotSpiderIK m_spiderIK;
    /// IKに必要な関節を取得できたか。失敗時は素材のアニメーションを使う。
    bool m_ikReady = false;
    /// 死亡時に分離した各部品の姿勢と動き。
    RobotDebris m_debris;
    /// 通常の機体の頂点を保持するGPUバッファ。
    VertexBuffer<SkinnedVertex> m_vertices;
    /// 死亡時の姿勢を焼き付けた破片の頂点。
    VertexBuffer<SkinnedVertex> m_debrisVertices;
    /// 機体と破片に共通のベースカラー画像。
    Texture m_albedo;
    /// 骨行列で頂点を動かすシェーダー。
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    /// テクスチャと光から色を計算するシェーダー。
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    /// SkinnedVertexの各項目をGPUへ伝える定義。
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
    /// 毎フレーム更新する骨行列の定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_boneBuffer;
    /// 1回の描画で使用する頂点数。本体と破片で共通。
    unsigned m_vertexCount = 0;
    /// 素材と描画資源が揃い、更新・描画できるか。
    bool m_ready = false;

    static SkinnedVertex MakeVertex(const RobotAnimationRig::Vertex& source);
    DirectX::SimpleMath::Matrix GetWorldMatrix() const;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;

    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override {} // CPlayerが決めた移動方向と経過秒数でAdvanceを呼ぶ。

    // deltaSecondsは前回からの経過秒。proceduralがtrueなら素材の歩行ではなくIKで足を動かす。
    void Advance(RobotMotion motion, float deltaSeconds, float playbackRate, float upperBodyYaw,
                 float upperBodyPitch, float groundY, bool grounded, bool procedural = true);
    void BeginBreakApart(float groundY);
    void AdvanceBreakApart(float deltaSeconds);
    void RestoreBody(); // DebugでHPを戻した場合も、素材を再読込せず元の機体へ戻す。
#ifdef _DEBUG
    void DrawIKDebugUI(); // Skill Debugから呼び、一時停止中も足の配置を調整する。
#endif
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
};
