#include "CMiniMap.h"
#include "Renderer.h"
#include "Application.h"
#include "Game.h"
#include "CPlayer.h"
#include "CEnemy.h"

using namespace DirectX::SimpleMath;

CMiniMap::CMiniMap() {
}

CMiniMap::~CMiniMap() {
    Uninit();
}

void CMiniMap::Init() {
    // 頂点の作成（中心基準の四角形）
    m_Vertices.resize(4);
    m_Vertices[0].position = Vector3(-0.5f, 0.5f, 0);
    m_Vertices[1].position = Vector3(0.5f, 0.5f, 0);
    m_Vertices[2].position = Vector3(-0.5f, -0.5f, 0);
    m_Vertices[3].position = Vector3(0.5f, -0.5f, 0);
    m_Vertices[0].uv = Vector2(0, 0);
    m_Vertices[1].uv = Vector2(1, 0);
    m_Vertices[2].uv = Vector2(0, 1);
    m_Vertices[3].uv = Vector2(1, 1);

    // ★修正：枠用の頂点は「白色」で作る
    for (int i = 0; i < 4; i++) m_Vertices[i].color = Color(1, 1, 1, 1);
    m_VertexBufferBg.Create(m_Vertices);

    // ★修正：敵の点用の頂点は「赤色」で作る
    for (int i = 0; i < 4; i++) m_Vertices[i].color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    m_VertexBufferEnemy.Create(m_Vertices);

    for (int i = 0; i < 4; i++) m_Vertices[i].color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    m_VertexBufferPlayer.Create(m_Vertices);

    m_Indices = { 0, 1, 2, 3 };
    m_IndexBuffer.Create(m_Indices);

    m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    m_TexBackground.Load("assets/texture/black.png");
    m_TexEnemy.Load("assets/texture/spark.png");
    m_TexPlayer.Load("assets/texture/icchann.png");

    m_Material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_Material->Create(mtrl);
}

void CMiniMap::Update() {
}

void CMiniMap::Draw(Camera* cam) {
    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0] == nullptr) return;

    Renderer::SetDepthEnable(false);
    cam->SetCamera(1);
    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    m_Shader.SetGPU();
    m_IndexBuffer.SetGPU();
    Renderer::SetUV(0, 0, 1, 1);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::SetBlendState(1);

    // ==========================================
    // 1. レーダー背景（枠）の描画
    // ==========================================
    m_VertexBufferBg.SetGPU();
    m_TexBackground.SetGPU();

    MATERIAL mtrlBg;
    mtrlBg.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    mtrlBg.TextureEnable = true;
    Renderer::SetMaterial(mtrlBg);

    // 背景は丸いので回転させず、そのまま所定の位置に描画
    Matrix worldMtxBg = Matrix::CreateScale(m_Scale) * Matrix::CreateTranslation(m_Position);
    Renderer::SetWorldMatrix(&worldMtxBg);
    dc->DrawIndexed(4, 0, 0);

    // ==========================================
    // 2. 敵の点の描画
    // ==========================================
    m_VertexBufferEnemy.SetGPU();
    m_TexEnemy.SetGPU();

    MATERIAL mtrlEnemy;
    mtrlEnemy.Diffuse = Color(1.0f, 0.0f, 0.0f, 1.0f); // 真っ赤に塗る
    mtrlEnemy.TextureEnable = true;
    Renderer::SetMaterial(mtrlEnemy);

    Vector3 playerPos = players[0]->GetPosition();
    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

    // CPlayerのMoveと同じ計算式で「カメラの前と右」を取得する！
    float angle = cam->GetCameraDirection();
    float fwdX = -sinf(angle);
    float fwdZ = -cosf(angle);
    float rightX = -cosf(angle);
    float rightZ = sinf(angle);

    for (auto& enemy : enemies) {
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
        if (dist > m_RadarRange) {
            localPt.x = (localPt.x / dist) * m_RadarRange;
            localPt.y = (localPt.y / dist) * m_RadarRange;
        }

        // -1.0 ～ 1.0 の割合に変換
        float mapX = localPt.x / m_RadarRange;
        float mapY = localPt.y / m_RadarRange;

        // レーダー上の最終的な画面座標
        Vector3 pointPos;
        pointPos.x = m_Position.x + (mapX * (m_Scale.x * 0.5f));
        pointPos.y = m_Position.y + (mapY * (m_Scale.y * 0.5f));
        pointPos.z = 0.0f;

        // 敵の点を配置して描画
        Matrix pt = Matrix::CreateTranslation(pointPos);
        Matrix ps = Matrix::CreateScale(15.0f, 15.0f, 1.0f);
        Matrix pWorld = ps * pt;

        Renderer::SetWorldMatrix(&pWorld);
        dc->DrawIndexed(4, 0, 0);
    }
    // ==========================================
   //3.Player
   // ==========================================
    m_VertexBufferPlayer.SetGPU(); // 
    m_TexPlayer.SetGPU();

    // プレイヤーは常にレーダーの中心
    Matrix ptPlayer = Matrix::CreateTranslation(m_Position);
    Matrix psPlayer = Matrix::CreateScale(20.0f, 20.0f, 1.0f);
    Matrix pWorldPlayer = psPlayer * ptPlayer;

    Renderer::SetWorldMatrix(&pWorldPlayer);
    dc->DrawIndexed(4, 0, 0);

    Renderer::SetDepthEnable(true);
}

void CMiniMap::Uninit() {
}
