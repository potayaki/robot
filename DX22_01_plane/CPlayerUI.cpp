#include "CPlayerUI.h"
#include "Renderer.h"
#include "Application.h"

CPlayerUI::CPlayerUI() {
    m_hpRate = 1.0f;       // 最初は満タン
    m_missileRate = 1.0f;  // 最初は満タン
}

CPlayerUI::~CPlayerUI() {
    Uninit();
}

void CPlayerUI::Init() {
    // 頂点の作成（中心を基準にした四角形）
    m_Vertices.resize(4);
    m_Vertices[0].position = DirectX::SimpleMath::Vector3(-0.5f, 0.5f, 0);
    m_Vertices[1].position = DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0);
    m_Vertices[2].position = DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0);
    m_Vertices[3].position = DirectX::SimpleMath::Vector3(0.5f, -0.5f, 0);

    for (int i = 0; i < 4; i++) {
        m_Vertices[i].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    }

    m_Vertices[0].uv = DirectX::SimpleMath::Vector2(0, 0);
    m_Vertices[1].uv = DirectX::SimpleMath::Vector2(1, 0);
    m_Vertices[2].uv = DirectX::SimpleMath::Vector2(0, 1);
    m_Vertices[3].uv = DirectX::SimpleMath::Vector2(1, 1);

    m_VertexBuffer.Create(m_Vertices);

    // インデックスの作成
    m_Indices.resize(4);
    m_Indices[0] = 0; m_Indices[1] = 1; m_Indices[2] = 2; m_Indices[3] = 3;
    m_IndexBuffer.Create(m_Indices);

    // 専用シェーダーを読み込む
    m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/UI_RingPS.hlsl");

    // 画像を読み込む
    m_TextureHP.Load("assets/texture/ring_ui2.png");
    m_TextureMissile.Load("assets/texture/ring_ui1.png");

    // GPUにデータを送るための定数バッファを作成
    Renderer::CreateConstantBuffer(sizeof(UIBuffer), &m_pUIBuffer);
}

void CPlayerUI::Update() {
    //TODO : Debug用にHPとミサイルの割合を減らしていく
    m_hpRate -= 0.005f;
    if (m_hpRate < 0.0f) m_hpRate = 1.0f;

    m_missileRate -= 0.01f;
    if (m_missileRate < 0.0f) m_missileRate = 1.0f;
}

void CPlayerUI::Draw(Camera* cam) {
    Renderer::SetDepthEnable(false);
    cam->SetCamera(1);

    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    // 共通の描画セット
    m_Shader.SetGPU();
    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();
    Renderer::SetUV(0, 0, 1, 1);
    Renderer::SetBlendState(1);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 共通の平行移動（位置）
    DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_Position);


    // -----------------------------------------------------
    // ① HPゲージ（外側：赤）の描画
    // -----------------------------------------------------
    DirectX::SimpleMath::Matrix s1 = DirectX::SimpleMath::Matrix::CreateScale(m_Scale);
    DirectX::SimpleMath::Matrix worldMtx1 = s1 * t;
    Renderer::SetWorldMatrix(&worldMtx1);

    UIBuffer uibufHP;
    uibufHP.Rate = m_hpRate;
    uibufHP.Color = DirectX::SimpleMath::Vector3(1.0f, 0.05f, 0.1f); // 赤色
    dc->UpdateSubresource(m_pUIBuffer, 0, NULL, &uibufHP, 0, 0);
    dc->PSSetConstantBuffers(6, 1, &m_pUIBuffer);

    m_TextureHP.SetGPU();
    dc->DrawIndexed((UINT)m_Indices.size(), 0, 0);


   
    // ② ミサイルゲージ（内側：青）の描画
    // ★ 1倍
    DirectX::SimpleMath::Matrix s2 = DirectX::SimpleMath::Matrix::CreateScale(m_Scale.x * 1.0f, m_Scale.y * 1.0f, 1.0f);
    DirectX::SimpleMath::Matrix worldMtx2 = s2 * t;
    Renderer::SetWorldMatrix(&worldMtx2);

    UIBuffer uibufMissile;
    uibufMissile.Rate = m_missileRate;
    uibufMissile.Color = DirectX::SimpleMath::Vector3(0.0f, 0.6f, 1.0f); // 青色
    dc->UpdateSubresource(m_pUIBuffer, 0, NULL, &uibufMissile, 0, 0);
    // (※さっきセットしたので PSSetConstantBuffers は省略可能ですが念のため)

    m_TextureMissile.SetGPU();
    dc->DrawIndexed((UINT)m_Indices.size(), 0, 0);


    Renderer::SetDepthEnable(true);
}

void CPlayerUI::Uninit() {
    SAFE_RELEASE(m_pUIBuffer);
}
