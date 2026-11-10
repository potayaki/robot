#pragma once
#include "Object.h"
#include "RobotAnimationRig.h"
#include "RobotDebris.h"
#include "RobotSpiderIK.h"
#include "Texture.h"
#include "VertexBuffer.h"

// プレイヤー専用の表示。本体の移動・HP・当たり判定はCPlayerが引き続き管理する。
class PlayerRobotModel : public Object
{
    static constexpr size_t MaxBones = 128;
    struct SkinnedVertex
    {
        VERTEX_3D vertex;
        unsigned bone;
    };
    RobotAnimationRig m_rig;
    RobotSpiderIK m_spiderIK;
    bool m_ikReady = false;
    RobotDebris m_debris;
    VertexBuffer<SkinnedVertex> m_vertices;
    VertexBuffer<SkinnedVertex> m_debrisVertices;
    Texture m_albedo;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_boneBuffer;
    unsigned m_vertexCount = 0;
    bool m_ready = false;

    static SkinnedVertex MakeVertex(const RobotAnimationRig::Vertex& source);
    DirectX::SimpleMath::Matrix GetWorldMatrix() const;

public:
    void Init() override;
    void Update() override {} // CPlayerが決めた移動方向と経過秒数でAdvanceを呼ぶ。
    void Advance(RobotMotion motion, float deltaSeconds, float playbackRate,
                 float upperBodyYaw, float upperBodyPitch, float groundY, bool grounded, bool procedural = true);
    void BeginBreakApart(float groundY);
    void AdvanceBreakApart(float deltaSeconds);
    void RestoreBody(); // DebugでHPを戻した場合も、素材を再読込せず元の機体へ戻す。
    void Draw(Camera* cam) override;
    void Uninit() override;
};
