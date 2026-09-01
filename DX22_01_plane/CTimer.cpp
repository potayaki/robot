#include "CTimer.h"
#include"Game.h"
using namespace DirectX::SimpleMath;

CTimer::CTimer() {
}

CTimer::~CTimer() {
}

void CTimer::Init() {
    m_Vertices.resize(4);
    m_Vertices[0].position = Vector3(-0.5f, 0.5f, 0);
    m_Vertices[1].position = Vector3(0.5f, 0.5f, 0);
    m_Vertices[2].position = Vector3(-0.5f, -0.5f, 0);
    m_Vertices[3].position = Vector3(0.5f, -0.5f, 0);
    for (int i = 0; i < 4; i++) m_Vertices[i].color = Color(1, 1, 1, 1);
    m_Vertices[0].uv = Vector2(0, 0);
    m_Vertices[1].uv = Vector2(1, 0);
    m_Vertices[2].uv = Vector2(0, 1);
    m_Vertices[3].uv = Vector2(1, 1);

    m_VertexBuffer.Create(m_Vertices);
    m_Indices = { 0, 1, 2, 3 };
    m_IndexBuffer.Create(m_Indices);

    m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    // 読み込む（パスは実際の場所に合わせる）
    m_TexNumber.Load("assets/texture/number.png");

    m_Material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_Material->Create(mtrl);
}

void CTimer::Update() {
    Renderer::SetDepthEnable(false);

    Renderer::SetDepthEnable(true);
}

void CTimer::Draw(Camera* cam) {
    Renderer::SetDepthEnable(false);
    cam->SetCamera(1); // 2Dカメラ
    ID3D11DeviceContext* dc = Renderer::GetDeviceContext();

    m_Shader.SetGPU();
    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();
    m_TexNumber.SetGPU();
    m_Material->SetGPU();
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::SetBlendState(1);

  
    // 特定の数字(0〜9)を、指定した位置とサイズで描画するラムダ関数
   
    auto drawDigit = [&](int digit, float x, float y, float w, float h) {
        Matrix world = Matrix::CreateScale(w, h, 1.0f) * Matrix::CreateTranslation(x, y, 0.0f);
        Renderer::SetWorldMatrix(&world);

        // 10等分したUV座標を計算して画像を切り抜く
        float uStart = digit * 0.1f;
        Renderer::SetUV(uStart, 0.0f, 0.1f, 1.0f);

        dc->DrawIndexed(4, 0, 0);
        };

    // ゲームの経過時間を取得
    float time = Game::GetInstance()->m_gameTime;

    // 各桁の数字を計算（例：12.34秒 の場合）
    int tens = ((int)time / 10) % 10;          // 10の位 (1)
    int ones = ((int)time) % 10;               // 1の位 (2)
    int dec1 = ((int)(time * 10.0f)) % 10;     // 小数第1位 (3)
    int dec2 = ((int)(time * 100.0f)) % 10;    // 小数第2位 (4)

    float baseX = m_Position.x;
    float baseY = m_Position.y;
    float w = m_Scale.x; // 数字1つの幅
    float h = m_Scale.y; // 数字1つの高さ
    float gap = w * 0.8f; // 数字同士の間隔

    // 秒数の描画
    drawDigit(tens, baseX - gap * 1.5f, baseY, w, h);
    drawDigit(ones, baseX - gap * 0.5f, baseY, w, h);

    // 小数（ミリ秒）の描画。少し小さくして、少し右に離す
    drawDigit(dec1, baseX + gap * 0.8f, baseY + (h * 0.1f), w * 0.8f, h * 0.8f);
    drawDigit(dec2, baseX + gap * 1.6f, baseY + (h * 0.1f), w * 0.8f, h * 0.8f);

    // ★重要：描画が終わったらUVを元に戻す（他の画像が切り抜かれるのを防ぐため）
    Renderer::SetUV(0.0f, 0.0f, 1.0f, 1.0f);
    Renderer::SetDepthEnable(true);
}

void CTimer::Uninit() {
}
