#pragma once
#include "Object.h"
#include "RobotAnimationRig.h"
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
    VertexBuffer<SkinnedVertex> m_vertices;
    Texture m_albedo;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_boneBuffer;
    unsigned m_vertexCount = 0;
    bool m_ready = false;

public:
    void Init() override;
    void Update() override {} // CPlayerが決めた移動方向と経過秒数でAdvanceを呼ぶ。
    void Advance(RobotMotion motion, float deltaSeconds, float playbackRate);
    void Draw(Camera* cam) override;
    void Uninit() override;
};
