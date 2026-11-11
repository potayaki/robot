/// @file CPlayerUI.cpp
/// @brief HPとミサイル待ち時間をリング状に表示するUI。

#include "CPlayerUI.h"
#include "Renderer.h"
#include "Application.h"

#include "Game.h"
#include "CPlayer.h"

CPlayerUI::CPlayerUI() {
    m_hpRate = 1.0f; // 最初は満タン
    m_missileRate = 1.0f; // 最初は満タン
}

CPlayerUI::~CPlayerUI() {
    Uninit();
}

void CPlayerUI::Init() {
    // 頂点の作成（中心を基準にした四角形）
    m_vertices.resize(4);
    m_vertices[0].position = DirectX::SimpleMath::Vector3(-0.5f, 0.5f, 0);
    m_vertices[1].position = DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0);
    m_vertices[2].position = DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0);
    m_vertices[3].position = DirectX::SimpleMath::Vector3(0.5f, -0.5f, 0);

    for (int i = 0; i < 4; i++) {
        m_vertices[i].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    }

    m_vertices[0].uv = DirectX::SimpleMath::Vector2(0, 0);
    m_vertices[1].uv = DirectX::SimpleMath::Vector2(1, 0);
    m_vertices[2].uv = DirectX::SimpleMath::Vector2(0, 1);
    m_vertices[3].uv = DirectX::SimpleMath::Vector2(1, 1);

    m_vertexBuffer.Create(m_vertices);

    // インデックスの作成
    m_indices.resize(4);
    m_indices[0] = 0;
    m_indices[1] = 1;
    m_indices[2] = 2;
    m_indices[3] = 3;
    m_indexBuffer.Create(m_indices);

    // 専用シェーダーを読み込む
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/UI_RingPS.hlsl");

    // 画像を読み込む
    m_textureHP.Load("assets/texture/ring_ui2.png");
    m_textureMissile.Load("assets/texture/ring_ui1.png");

    // GPUにデータを送るための定数バッファを作成
    Renderer::CreateConstantBuffer(sizeof(UIBuffer), &m_pUIBuffer);
}

void CPlayerUI::Update() {
    //TODO : Debug用にHPとミサイルの割合を減らしていく
    /*
    m_hpRate -= 0.005f;
    if (m_hpRate < 0.0f) m_hpRate = 1.0f;

    m_missileRate -= 0.01f;
    if (m_missileRate < 0.0f) m_missileRate = 1.0f;
    */
    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();
    if (!players.empty() && players[0] != nullptr) {
        CPlayer* player = players[0];

        // ① HPの割合を計算 (現在のHP ÷ 最大HP)
        m_hpRate = (float)player->GetHp() / (float)player->GetMaxHp();

        // ② ミサイルの割合を計算
        // m_currentMissileTime は「撃てるまでの残り秒数(2.0→0)」なので、
        // 0の時にゲージ満タン(1.0)になるように逆算する
        float maxTime = player->GetMissileTime();
        float curTime = player->GetCurrentMissileTime();
        if (maxTime > 0.0f) {
            m_missileRate = 1.0f - (curTime / maxTime);
        }

        // 値が 0.0 ～ 1.0 の範囲外に行かないようにクランプ（安全対策）
        if (m_hpRate < 0.0f) {
            m_hpRate = 0.0f;
        }
        if (m_hpRate > 1.0f) {
            m_hpRate = 1.0f;
        }
        if (m_missileRate < 0.0f) {
            m_missileRate = 0.0f;
        }
        if (m_missileRate > 1.0f) {
            m_missileRate = 1.0f;
        }
    }
}

void CPlayerUI::Draw(Camera* cam) {
    Renderer::SetDepthEnable(false);
    cam->SetCamera(1);

    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    // 共通の描画セット
    m_shader.SetGPU();
    m_vertexBuffer.SetGPU();
    m_indexBuffer.SetGPU();
    Renderer::SetUV(0, 0, 1, 1);
    Renderer::SetBlendState(BS_ALPHABLEND);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 共通の平行移動（位置）
    DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_position);

    // -----------------------------------------------------
    // ① HPゲージ（外側：赤）の描画
    // -----------------------------------------------------
    DirectX::SimpleMath::Matrix s1 = DirectX::SimpleMath::Matrix::CreateScale(m_scale);
    DirectX::SimpleMath::Matrix worldMtx1 = s1 * t;
    Renderer::SetWorldMatrix(&worldMtx1);

    UIBuffer uibufHP;
    uibufHP.Rate = m_hpRate;
    uibufHP.Color = DirectX::SimpleMath::Vector3(1.0f, 0.05f, 0.1f); // 赤色
    dc->UpdateSubresource(m_pUIBuffer, 0, NULL, &uibufHP, 0, 0);
    dc->PSSetConstantBuffers(6, 1, &m_pUIBuffer);

    m_textureHP.SetGPU();
    dc->DrawIndexed((UINT)m_indices.size(), 0, 0);

    // ② ミサイルゲージ（内側：青）の描画
    // ★ 1倍
    DirectX::SimpleMath::Matrix s2 =
        DirectX::SimpleMath::Matrix::CreateScale(m_scale.x * 1.0f, m_scale.y * 1.0f, 1.0f);
    DirectX::SimpleMath::Matrix worldMtx2 = s2 * t;
    Renderer::SetWorldMatrix(&worldMtx2);

    UIBuffer uibufMissile;
    uibufMissile.Rate = m_missileRate;
    uibufMissile.Color = DirectX::SimpleMath::Vector3(0.0f, 0.6f, 1.0f); // 青色
    dc->UpdateSubresource(m_pUIBuffer, 0, NULL, &uibufMissile, 0, 0);
    // (※さっきセットしたので PSSetConstantBuffers は省略可能ですが念のため)

    m_textureMissile.SetGPU();
    dc->DrawIndexed((UINT)m_indices.size(), 0, 0);

    Renderer::SetDepthEnable(true);
}

void CPlayerUI::Uninit() {
    SAFE_RELEASE(m_pUIBuffer);
}
