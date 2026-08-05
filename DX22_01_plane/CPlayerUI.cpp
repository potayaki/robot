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
    m_Texture.Load("assets/texture/ring_ui.png");

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
    // 2D描画モードに切り替え
    Renderer::SetDepthEnable(false);
    cam->SetCamera(1);

    // 行列の計算
    DirectX::SimpleMath::Matrix s = DirectX::SimpleMath::Matrix::CreateScale(m_Scale);
    DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_Position);
    DirectX::SimpleMath::Matrix worldMtx = s * t; // 2Dなので回転は省略
    Renderer::SetWorldMatrix(&worldMtx);

    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    // ★送るデータをセットして、GPU（シェーダー）の6番スロットに送信！
    UIBuffer uibuf;
    uibuf.hpRate = m_hpRate;
    uibuf.missileRate = m_missileRate;
    dc->UpdateSubresource(m_pUIBuffer, 0, NULL, &uibuf, 0, 0);
    dc->PSSetConstantBuffers(6, 1, &m_pUIBuffer);

    // 描画セット
    m_Shader.SetGPU();
    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();
    m_Texture.SetGPU();

    Renderer::SetUV(0, 0, 1, 1);

    // UIなので半透明合成(BS_ALPHABLEND)にする
    Renderer::SetBlendState(1);

    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    dc->DrawIndexed((UINT)m_Indices.size(), 0, 0);

    Renderer::SetDepthEnable(true);
}

void CPlayerUI::Uninit() {
    SAFE_RELEASE(m_pUIBuffer);
}
