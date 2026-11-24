/// @file CParticle.cpp
/// @brief 火花などの粒子の移動、寿命、共有描画資源。

#include "CParticle.h"
#include "Game.h"
#include "Renderer.h"
#include "TestCube.h"
#include "ShotParticleMesh.h"
#include <cmath>
#include <optional>

using namespace DirectX::SimpleMath;

namespace {
    struct ParticleResources {
        /// 破片粒子で共有する立方体モデル。
        TestCube m_body;
        /// 火花粒子で共有する画像とシェーダー。
        Texture m_sparkTexture;
        Shader m_sparkShader;
        /// 火花を描画する板の頂点とインデックス。
        VertexBuffer<VERTEX_3D> m_sparkVB;
        IndexBuffer m_sparkIB;
        /// 四種類の射撃演出をまとめた三角形リスト。テクスチャを使用しない。
        VertexBuffer<VERTEX_3D> m_shotVertices;
        /// 描画する種類ごとの頂点範囲。
        ShotParticleMesh::Range m_plume, m_plumeGlow, m_ribbon, m_chip, m_ring;
        /// 粒子の縁色・芯色・透明度を転送するピクセル用定数バッファ。
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_material;
        /// 地形で火花を遮蔽し、粒子同士は遮らない深度設定。
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_sparkDepthState;

        ParticleResources();
    };

    std::optional<ParticleResources> resources;

    Matrix AlongAxis(Vector3 axis) {
        if (axis.LengthSquared() < 0.000001f) {
            return Matrix::Identity;
        }
        axis.Normalize();
        const auto reference = std::abs(axis.y) < 0.95f ? Vector3::UnitY : Vector3::UnitZ;
        auto side = axis.Cross(reference);
        side.Normalize();
        return Matrix(axis, side.Cross(axis), side);
    }
} // namespace

CParticle::CParticle() {
    if (m_type == Test) {
        m_life = 1.0f;
    }

    m_color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
}

CParticle::~CParticle() {
    Uninit();
}

void CParticle::Init() {
    Preload();
}

void CParticle::Update() {
    const float deltaSeconds = (std::min)(GameTime::DeltaSeconds(), m_life);

    if (m_type == Test || m_type == ImpactChip || m_type == MuzzleSpark) {
        constexpr float chipGravity = 260.0f;
        const float gravity = m_type == ImpactChip ? chipGravity : m_gravity;
        m_position += m_velocity * deltaSeconds;
        m_position.y -= 0.5f * gravity * deltaSeconds * deltaSeconds;
        m_velocity.y -= gravity * deltaSeconds;
    }
    if (m_type == Test) {
        // クルクル回転させる
        m_rotation.x += m_spinSpeed * deltaSeconds;
        m_rotation.y += m_spinSpeed * deltaSeconds;

    } else if (m_type == Spark || m_type == MuzzleSpark || m_type == MuzzleFlash) {
        // 旧「毎フレーム0.95倍」を秒に応じた指数減衰へ変換する。
        const float shrink = std::pow(m_shrinkPerReferenceFrame, deltaSeconds * m_referenceFramesPerSecond);
        m_scale *= shrink;
    }

    // 寿命を減らして、0になったら自分を消す
    GameTime::CountDown(m_life, deltaSeconds);
    if (m_life <= 0.0f) {
        SetActive(false);
    }
}

void CParticle::Draw(Camera* cam) {
    if (!resources || !cam) {
        return;
    }
    if (m_type == Test) {
        resources->m_body.SetPosition(m_position.x, m_position.y, m_position.z);
        resources->m_body.SetRotation(m_rotation);
        resources->m_body.SetScale(m_scale.x, m_scale.y, m_scale.z);
        resources->m_body.Draw(cam);
        return;
    }
    const bool textured = m_type == Spark || m_type == MuzzleSpark || m_type == MuzzleFlash;
    const float life = m_initialLife > 0 ? std::clamp(m_life / m_initialLife, 0.0f, 1.0f) : 0;
    cam->SetCamera(0);
    Matrix rotation;
    if (textured || m_type == ImpactRing) {
        rotation = cam->GetViewMatrix().Invert();
        rotation.Translation(Vector3::Zero);
    } else {
        rotation = AlongAxis(m_velocity);
    }
    Vector3 scale = m_scale;
    MATERIAL material{};
    material.Diffuse = m_color;
    material.TextureEnable = textured;
    ShotParticleMesh::Range range;
    switch (m_type) {
        case MuzzlePlume:
            // 起点は銃口に固定し、先端から引き戻す。芯は白黄、外縁は発生器の橙。
            scale.x *= 0.45f + 0.55f * life;
            scale.y *= 0.55f + 0.45f * life;
            scale.z = scale.y;
            material.Emmision = Color(1, 1, 0.86f, 1);
            material.Diffuse.w *= (std::min)(1.0f, 1.8f * life);
            range = resources->m_plume;
            break;
        case BulletRibbon:
            // 端点の位置は寿命中一定。幅と不透明度だけを落として実際の軌跡を残す。
            scale.y *= 0.65f + 0.35f * life;
            scale.z = scale.y;
            material.Emmision = Color(0.9f, 0.98f, 1.0f, 1);
            material.Diffuse.w *= life;
            range = resources->m_ribbon;
            break;
        case ImpactChip:
            scale.z = scale.y;
            material.Emmision = Color(1, 0.62f, 0.71f, 1);
            material.Diffuse.w *= (std::min)(1.0f, 1.5f * life);
            range = resources->m_chip;
            break;
        case ImpactRing:
            scale *= 1.0f - 0.68f * life * life;
            material.Emmision = Color(1, 0.30f, 0.50f, 1);
            // 開く途中の細い縁をはっきり残し、寿命の最後の4割で消す。
            material.Diffuse.w *= (std::min)(1.0f, 2.5f * life);
            range = resources->m_ring;
            break;
        case MuzzleFlash:
        case MuzzleSpark:
            material.Diffuse.w *= life;
            break;
    }
    Matrix world = Matrix::CreateScale(scale) * rotation * Matrix::CreateTranslation(m_position);
    resources->m_sparkShader.SetGPU();

    auto dc = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11Buffer> previousMaterial;
    dc->PSGetConstantBuffers(4, 1, previousMaterial.GetAddressOf());
    dc->UpdateSubresource(resources->m_material.Get(), 0, nullptr, &material, 0, 0);
    dc->PSSetConstantBuffers(4, 1, resources->m_material.GetAddressOf());
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    UINT previousStencilRef = 0, previousSampleMask = 0;
    FLOAT previousFactors[4]{};
    dc->OMGetDepthStencilState(previousDepth.GetAddressOf(), &previousStencilRef);
    dc->OMGetBlendState(previousBlend.GetAddressOf(), previousFactors, &previousSampleMask);
    dc->OMSetDepthStencilState(resources->m_sparkDepthState.Get(), 0);
    Renderer::SetBlendState(textured ? BS_ADDITIVE : BS_ALPHABLEND);
    if (textured) {
        Renderer::SetWorldMatrix(&world);
        Renderer::SetUV(0, 0, 1, 1);
        resources->m_sparkVB.SetGPU();
        resources->m_sparkIB.SetGPU();
        resources->m_sparkTexture.SetGPU();
        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        dc->DrawIndexed(4, 0, 0);
    } else {
        resources->m_shotVertices.SetGPU();
        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        if (m_type == MuzzlePlume) {
            // 芯は銃口サイズのまま。軸の真後ろからも見える周辺光を先に一度だけ重ねる。
            constexpr float glowWidths = 3.6f;
            const float diameter = scale.y * glowWidths;
            Matrix billboard = cam->GetViewMatrix().Invert();
            billboard.Translation(m_position + rotation.Right() * (scale.x * 0.26f));
            Matrix glowWorld = Matrix::CreateScale(diameter, diameter, 1.0f) * billboard;
            Renderer::SetWorldMatrix(&glowWorld);
            Renderer::SetBlendState(BS_ADDITIVE);
            dc->Draw(resources->m_plumeGlow.count, resources->m_plumeGlow.first);
            Renderer::SetBlendState(BS_ALPHABLEND);
        }
        Renderer::SetWorldMatrix(&world);
        dc->Draw(range.count, range.first);
    }
    dc->OMSetBlendState(previousBlend.Get(), previousFactors, previousSampleMask);
    dc->OMSetDepthStencilState(previousDepth.Get(), previousStencilRef);
    dc->PSSetConstantBuffers(4, 1, previousMaterial.GetAddressOf());
}

void CParticle::Uninit() {}

void CParticle::Preload() {
    if (!resources) {
        resources.emplace();
    }
}

void CParticle::UnloadResources() {
    resources.reset();
}

ParticleResources::ParticleResources() {
    m_body.Init();
    m_body.SetScale(0.5f, 0.5f, 0.5f);
    m_sparkTexture.Load("assets/texture/spark.png");
    m_sparkShader.Create("shader/unlitTextureVS.hlsl", "shader/particlePS.hlsl");
    Renderer::CreateConstantBuffer(sizeof(MATERIAL), m_material.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = TRUE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    Renderer::GetDevice()->CreateDepthStencilState(&depth, m_sparkDepthState.GetAddressOf());

    // 板の頂点（4つの角）
    std::vector<VERTEX_3D> vertices(4);
    vertices[0].position = DirectX::SimpleMath::Vector3(-0.5f, 0.5f, 0);
    vertices[1].position = DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0);
    vertices[2].position = DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0);
    vertices[3].position = DirectX::SimpleMath::Vector3(0.5f, -0.5f, 0);
    for (int i = 0; i < 4; i++) {
        vertices[i].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    }
    vertices[0].uv = DirectX::SimpleMath::Vector2(0, 0);
    vertices[1].uv = DirectX::SimpleMath::Vector2(1, 0);
    vertices[2].uv = DirectX::SimpleMath::Vector2(0, 1);
    vertices[3].uv = DirectX::SimpleMath::Vector2(1, 1);

    m_sparkVB.Create(vertices);

    std::vector<unsigned int> indices = {0, 1, 2, 3};
    m_sparkIB.Create(indices);

    vertices.clear();
    m_plume = ShotParticleMesh::Plume(vertices);
    m_plumeGlow = ShotParticleMesh::PlumeGlow(vertices);
    m_ribbon = ShotParticleMesh::Ribbon(vertices);
    m_chip = ShotParticleMesh::Chip(vertices);
    m_ring = ShotParticleMesh::Ring(vertices);
    m_shotVertices.Create(vertices);
}
