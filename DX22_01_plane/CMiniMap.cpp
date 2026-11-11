/// @file CMiniMap.cpp
/// @brief プレイヤーと敵の位置を表示するミニマップ。

#include "CMiniMap.h"
#include "Renderer.h"
#include "Application.h"
#include "Game.h"
#include "CPlayer.h"
#include "EnemyTarget.h"

using namespace DirectX::SimpleMath;

CMiniMap::CMiniMap() {}

CMiniMap::~CMiniMap() {
    Uninit();
}

void CMiniMap::Init() {
    // 頂点の作成（中心基準の四角形）
    m_vertices.resize(4);
    m_vertices[0].position = Vector3(-0.5f, 0.5f, 0);
    m_vertices[1].position = Vector3(0.5f, 0.5f, 0);
    m_vertices[2].position = Vector3(-0.5f, -0.5f, 0);
    m_vertices[3].position = Vector3(0.5f, -0.5f, 0);
    m_vertices[0].uv = Vector2(0, 0);
    m_vertices[1].uv = Vector2(1, 0);
    m_vertices[2].uv = Vector2(0, 1);
    m_vertices[3].uv = Vector2(1, 1);

    // 外枠の色にする。
    for (int i = 0; i < 4; i++) {
        m_vertices[i].color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    }
    m_vertexBufferBg.Create(m_vertices);

    // 敵の点用の頂点は「赤色」で作る
    for (int i = 0; i < 4; i++) {
        m_vertices[i].color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    }
    m_vertexBufferEnemy.Create(m_vertices);

    for (int i = 0; i < 4; i++) {
        m_vertices[i].color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    }
    m_vertexBufferPlayer.Create(m_vertices);

    m_indices = {0, 1, 2, 3};
    m_indexBuffer.Create(m_indices);

    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    m_texBackground.Load("assets/texture/black.png");
    m_texFrame.Load("assets/texture/white.png");
    m_texEnemy.Load("assets/texture/spark.png");
    m_texPlayer.Load("assets/texture/icchann.png");

    m_material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_material->Create(mtrl);
}

void CMiniMap::Update() {}

void CMiniMap::Draw(Camera* cam) {
    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0] == nullptr) {
        return;
    }

    Renderer::SetDepthEnable(false);
    cam->SetCamera(1);
    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    m_shader.SetGPU();
    m_indexBuffer.SetGPU();
    Renderer::SetUV(0, 0, 1, 1);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::SetBlendState(BS_ALPHABLEND);

    // ==========================================
    // 1. レーダーの外枠と背景の描画
    // ==========================================
    // 外側を青灰色で描き、黒い空との境界をはっきりさせる。
    m_vertexBufferBg.SetGPU();
    m_texFrame.SetGPU();

    MATERIAL mtrlBg;
    mtrlBg.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    mtrlBg.TextureEnable = true;
    Renderer::SetMaterial(mtrlBg);

    // 背景より大きい四角形を先に描き、外側の約2ピクセルを細い枠として残す。
    Matrix worldMtxBg = Matrix::CreateScale(m_scale) * Matrix::CreateTranslation(m_position);
    Renderer::SetWorldMatrix(&worldMtxBg);
    dc->DrawIndexed(4, 0, 0);

    // 枠の内側だけ従来の黒背景で覆い、赤い敵の点の視認性を保つ。
    m_vertexBufferPlayer.SetGPU();
    m_texBackground.SetGPU();
    Matrix innerScale = Matrix::CreateScale(m_scale.x * 0.98f, m_scale.y * 0.98f, m_scale.z);
    Matrix worldMtxInner = innerScale * Matrix::CreateTranslation(m_position);
    Renderer::SetWorldMatrix(&worldMtxInner);
    dc->DrawIndexed(4, 0, 0);

    // ==========================================
    // 2. 敵の点の描画
    // ==========================================
    // spark.png は黒背景付きなので、敵の点だけ加算合成して黒い四角を描かない。
    Renderer::SetBlendState(BS_ADDITIVE);
    m_vertexBufferEnemy.SetGPU();
    m_texEnemy.SetGPU();

    MATERIAL mtrlEnemy;
    mtrlEnemy.Diffuse = Color(1.0f, 0.0f, 0.0f, 1.0f); // 真っ赤に塗る
    mtrlEnemy.TextureEnable = true;
    Renderer::SetMaterial(mtrlEnemy);

    Vector3 playerPos = players[0]->GetPosition();
    // 飛行ドローンも既存の敵マーカーとして表示する。
    std::vector<EnemyTarget*> enemies = Game::GetInstance()->GetObjects<EnemyTarget>();

    // CPlayerのMoveと同じ計算式で「カメラの前と右」を取得する！
    float angle = cam->GetCameraDirection();
    float fwdX = -sinf(angle);
    float fwdZ = -cosf(angle);
    float rightX = -cosf(angle);
    float rightZ = sinf(angle);

    for (auto& enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead()) {
            continue;
        }
        Vector3 diff = enemy->GetPosition() - playerPos;

        // 内積を使って、敵が「プレイヤーの前方向に何m」「右方向に何m」いるか直接測る
        float distFront = diff.x * fwdX + diff.z * fwdZ;
        float distRight = diff.x * rightX + diff.z * rightZ;

        // 画面上では Y軸のマイナス方向が「上」なので、前方向(Front)を反転させる
        Vector3 localPt;
        localPt.x = distRight;
        localPt.y = distFront;
        localPt.z = 0.0f;

        // レーダーの範囲内に収める（範囲外ならフチに張り付かせる）
        float dist = sqrtf(localPt.x * localPt.x + localPt.y * localPt.y);
        if (dist > m_radarRange) {
            localPt.x = (localPt.x / dist) * m_radarRange;
            localPt.y = (localPt.y / dist) * m_radarRange;
        }

        // -1.0 ～ 1.0 の割合に変換
        float mapX = localPt.x / m_radarRange;
        float mapY = localPt.y / m_radarRange;

        // レーダー上の最終的な画面座標
        Vector3 pointPos;
        pointPos.x = m_position.x + (mapX * (m_scale.x * 0.5f));
        pointPos.y = m_position.y + (mapY * (m_scale.y * 0.5f));
        pointPos.z = 0.0f;

        // 敵の点を配置して描画
        Matrix pt = Matrix::CreateTranslation(pointPos);
        Matrix ps = Matrix::CreateScale(15.0f, 15.0f, 1.0f);
        Matrix pWorld = ps * pt;

        Renderer::SetWorldMatrix(&pWorld);
        dc->DrawIndexed(4, 0, 0);
    }
    // プレイヤーの画像は通常の透明合成で描くため、加算合成をここで解除する。
    Renderer::SetBlendState(BS_ALPHABLEND);
    // ==========================================
    //3.Player
    // ==========================================
    m_vertexBufferPlayer.SetGPU(); //
    m_texPlayer.SetGPU();

    // プレイヤーは常にレーダーの中心
    Matrix ptPlayer = Matrix::CreateTranslation(m_position);
    Matrix psPlayer = Matrix::CreateScale(20.0f, 20.0f, 1.0f);
    Matrix pWorldPlayer = psPlayer * ptPlayer;

    Renderer::SetWorldMatrix(&pWorldPlayer);
    dc->DrawIndexed(4, 0, 0);

    Renderer::SetDepthEnable(true);
}

void CMiniMap::Uninit() {}
