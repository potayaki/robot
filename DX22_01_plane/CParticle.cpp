/// @file CParticle.cpp
/// @brief 火花などの粒子の移動、寿命、共有描画資源。

#include "CParticle.h"
#include "Game.h"
#include "Renderer.h"
#include "TestCube.h"
#include <cmath>
#include <optional>

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
        /// 板に設定する材質。
        Material m_sparkMaterial;
        /// 地形で火花を遮蔽し、粒子同士は遮らない深度設定。
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_sparkDepthState;

        ParticleResources();
    };

    std::optional<ParticleResources> resources;
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

    if (m_type == Test) {
        // 速度の分だけ移動

        m_position += m_velocity * deltaSeconds;
        m_position.y -= 0.5f * m_gravity * deltaSeconds * deltaSeconds;
        m_velocity.y -= m_gravity * deltaSeconds; // 重力（距離/秒²）。

        // クルクル回転させる
        m_rotation.x += m_spinSpeed * deltaSeconds;
        m_rotation.y += m_spinSpeed * deltaSeconds;

    } else if (m_type == Spark || m_type == MuzzleSpark || m_type == MuzzleFlash) {
        if (m_type == MuzzleSpark) {
            m_position += m_velocity * deltaSeconds;
            m_position.y -= 0.5f * m_gravity * deltaSeconds * deltaSeconds;
            m_velocity.y -= m_gravity * deltaSeconds;
        }
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
    if (!resources) {
        return;
    }
    if (m_type == Test) {
        resources->m_body.SetPosition(m_position.x, m_position.y, m_position.z);
        resources->m_body.SetRotation(m_rotation);
        resources->m_body.SetScale(m_scale.x, m_scale.y, m_scale.z);
        resources->m_body.Draw(cam);
    }
    //  Sparkの場合は、板（ビルボード）として画像を描く
    else if (m_type == Spark || m_type == MuzzleSpark || m_type == MuzzleFlash) {
        ID3D11DeviceContext* dc = Renderer::GetDeviceContext();
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
        UINT previousStencilRef = 0;
        dc->OMGetDepthStencilState(previousDepth.GetAddressOf(), &previousStencilRef);
        dc->OMSetDepthStencilState(resources->m_sparkDepthState.Get(), 0);
        Renderer::SetBlendState(BS_ADDITIVE); // 加算合成（光るモード）

        cam->SetCamera(0); // 3Dカメラを使用
        DirectX::SimpleMath::Matrix viewMtx = cam->GetViewMatrix();

        // カメラの逆行列を使って、板を常にカメラの真正面に
        DirectX::SimpleMath::Matrix billboardMtx = viewMtx.Invert();
        billboardMtx.Translation(DirectX::SimpleMath::Vector3::Zero);

        DirectX::SimpleMath::Matrix s = DirectX::SimpleMath::Matrix::CreateScale(m_scale);
        DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_position);
        DirectX::SimpleMath::Matrix worldMtx = s * billboardMtx * t;

        Renderer::SetWorldMatrix(&worldMtx);
        Renderer::SetUV(0, 0, 1, 1);

        resources->m_sparkShader.SetGPU();
        resources->m_sparkVB.SetGPU();
        resources->m_sparkIB.SetGPU();
        resources->m_sparkTexture.SetGPU();
        resources->m_sparkMaterial.SetGPU();

        MATERIAL mtrl{};
        mtrl.Diffuse = m_color; // パーティクルの色をマテリアルに反映
        if (m_type == MuzzleSpark || m_type == MuzzleFlash) {
            mtrl.Diffuse.w *= m_initialLife > 0.0f ? m_life / m_initialLife : 0.0f;
        }
        mtrl.Ambient = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
        mtrl.TextureEnable = true;
        Renderer::SetMaterial(mtrl);

        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        dc->DrawIndexed(4, 0, 0);

        Renderer::SetBlendState(BS_ALPHABLEND); // 通常の半透明に戻す
        dc->OMSetDepthStencilState(previousDepth.Get(), previousStencilRef);
    }
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

    MATERIAL mtrl{};
    mtrl.Diffuse = DirectX::SimpleMath::Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_sparkMaterial.Create(mtrl);
}
