#pragma once
#include "Shader.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include <array>

// 焼き付けた射撃ポーズ。台座の回転は呼び出し側の行列から一度だけ受け取る。
class TitleRobotFigure {
public:
    struct FirePose {
        float sensor = 0;
        float recoil = 0;
        float shotSeconds = 0;
    };

    bool Init();
    void Draw(const DirectX::SimpleMath::Matrix& world, const FirePose& pose);
    void DrawBodyDepth(const DirectX::SimpleMath::Matrix& world, float recoil);

private:
    VertexBuffer<VERTEX_3D> m_robot;

    struct Range {
        UINT first = 0, count = 0;
    };

    struct Gun {
        DirectX::SimpleMath::Vector3 socket, axis;
        Range barrel, flash, glow, sparks, sparkGlow, tracer;
    };

    /// 銃身を除く固定部分を先頭に、その後に左右の銃身を格納する。
    UINT m_bodyCount = 0, m_robotCount = 0;
    std::array<Gun, 2> m_guns{};
    /// 発光・火花・曳光弾の頂点は一度だけ作り、射撃時は描画行列で動かす。
    VertexBuffer<VERTEX_3D> m_effects;
    ComPtr<ID3D11Buffer> m_lightPulse;
    Texture m_albedo, m_metallic, m_emissive, m_roughness, m_white;
    Shader m_robotShader, m_flashShader, m_glowShader;
};
