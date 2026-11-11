/// @file TestCube.cpp
/// @brief 単純な立方体モデルの頂点作成と描画。

#include "TestCube.h"
#include "Object.h"
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void TestCube::Init() {
    // 頂点データ
    m_vertices.resize(32);
    m_vertices[0].position = Vector3(-10, 10, 10);
    m_vertices[1].position = Vector3(10, 10, 10);
    m_vertices[2].position = Vector3(-10, 10, -10);
    m_vertices[3].position = Vector3(10, 10, -10);

    m_vertices[0].color = Color(1, 1, 1, 1);
    m_vertices[1].color = Color(1, 1, 1, 1);
    m_vertices[2].color = Color(1, 1, 1, 1);
    m_vertices[3].color = Color(1, 1, 1, 1);

    m_vertices[0].uv = Vector2(0, 0);
    m_vertices[1].uv = Vector2(0.33f, 0);
    m_vertices[2].uv = Vector2(0, 0.5f);
    m_vertices[3].uv = Vector2(0.33f, 0.5f);

    m_vertices[0].normal = Vector3(0, 1, 0);
    m_vertices[1].normal = Vector3(0, 1, 0);
    m_vertices[2].normal = Vector3(0, 1, 0);
    m_vertices[3].normal = Vector3(0, 1, 0);

    m_vertices[4].position = Vector3(-10, 10, -10);
    m_vertices[5].position = Vector3(10, 10, -10);
    m_vertices[6].position = Vector3(-10, -10, -10);
    m_vertices[7].position = Vector3(10, -10, -10);

    m_vertices[4].color = Color(1, 1, 1, 1);
    m_vertices[5].color = Color(1, 1, 1, 1);
    m_vertices[6].color = Color(1, 1, 1, 1);
    m_vertices[7].color = Color(1, 1, 1, 1);

    m_vertices[4].uv = Vector2(0.33f, 0);
    m_vertices[5].uv = Vector2(0.66f, 0);
    m_vertices[6].uv = Vector2(0.33f, 0.5f);
    m_vertices[7].uv = Vector2(0.66f, 0.5f);

    m_vertices[4].normal = Vector3(0, 0, -1);
    m_vertices[5].normal = Vector3(0, 0, -1);
    m_vertices[6].normal = Vector3(0, 0, -1);
    m_vertices[7].normal = Vector3(0, 0, -1);

    m_vertices[8].position = Vector3(-10, 10, 10);
    m_vertices[9].position = Vector3(-10, 10, -10);
    m_vertices[10].position = Vector3(-10, -10, -10);
    m_vertices[11].position = Vector3(-10, -10, 10);

    m_vertices[8].color = Color(1, 1, 1, 1);
    m_vertices[9].color = Color(1, 1, 1, 1);
    m_vertices[10].color = Color(1, 1, 1, 1);
    m_vertices[11].color = Color(1, 1, 1, 1);

    m_vertices[9].uv = Vector2(0.66f, 0);
    m_vertices[8].uv = Vector2(1, 0);
    m_vertices[10].uv = Vector2(0.66f, 0.5f);
    m_vertices[11].uv = Vector2(1, 0.5f);

    m_vertices[8].normal = Vector3(-1, 0, 0);
    m_vertices[9].normal = Vector3(-1, 0, 0);
    m_vertices[10].normal = Vector3(-1, 0, 0);
    m_vertices[11].normal = Vector3(-1, 0, 0);

    m_vertices[12].position = Vector3(10, 10, 10);
    m_vertices[13].position = Vector3(-10, 10, 10);
    m_vertices[14].position = Vector3(-10, -10, 10);
    m_vertices[15].position = Vector3(10, -10, 10);

    m_vertices[12].color = Color(1, 1, 1, 1);
    m_vertices[13].color = Color(1, 1, 1, 1);
    m_vertices[14].color = Color(1, 1, 1, 1);
    m_vertices[15].color = Color(1, 1, 1, 1);

    m_vertices[13].uv = Vector2(0.33f, 0.5f);
    m_vertices[12].uv = Vector2(0.66f, 0.5f);
    m_vertices[14].uv = Vector2(0.33f, 1);
    m_vertices[15].uv = Vector2(0.66f, 1);

    m_vertices[12].normal = Vector3(0, 0, 1);
    m_vertices[13].normal = Vector3(0, 0, 1);
    m_vertices[14].normal = Vector3(0, 0, 1);
    m_vertices[15].normal = Vector3(0, 0, 1);

    m_vertices[16].position = Vector3(10, 10, -10);
    m_vertices[17].position = Vector3(10, 10, 10);
    m_vertices[18].position = Vector3(10, -10, 10);
    m_vertices[19].position = Vector3(10, -10, -10);

    m_vertices[16].color = Color(1, 1, 1, 1);
    m_vertices[17].color = Color(1, 1, 1, 1);
    m_vertices[18].color = Color(1, 1, 1, 1);
    m_vertices[19].color = Color(1, 1, 1, 1);

    m_vertices[17].uv = Vector2(0, 0.5f);
    m_vertices[16].uv = Vector2(0.33f, 0.5f);
    m_vertices[18].uv = Vector2(0, 1);
    m_vertices[19].uv = Vector2(0.33f, 1);

    m_vertices[16].normal = Vector3(1, 0, 0);
    m_vertices[17].normal = Vector3(1, 0, 0);
    m_vertices[18].normal = Vector3(1, 0, 0);
    m_vertices[19].normal = Vector3(1, 0, 0);

    m_vertices[20].position = Vector3(-10, -10, 10);
    m_vertices[21].position = Vector3(10, -10, 10);
    m_vertices[22].position = Vector3(10, -10, -10);
    m_vertices[23].position = Vector3(-10, -10, -10);

    m_vertices[20].color = Color(1, 1, 1, 1);
    m_vertices[21].color = Color(1, 1, 1, 1);
    m_vertices[22].color = Color(1, 1, 1, 1);
    m_vertices[23].color = Color(1, 1, 1, 1);

    m_vertices[21].uv = Vector2(0.66f, 0.5f);
    m_vertices[20].uv = Vector2(1, 0.5f);
    m_vertices[22].uv = Vector2(0.66f, 1);
    m_vertices[23].uv = Vector2(1, 1);

    m_vertices[20].normal = Vector3(0, -1, 0);
    m_vertices[21].normal = Vector3(0, -1, 0);
    m_vertices[22].normal = Vector3(0, -1, 0);
    m_vertices[23].normal = Vector3(0, -1, 0);
    // 頂点バッファ生成
    m_vertexBuffer.Create(m_vertices);

    // インデックデータ
    m_indices.resize(40);

    m_indices = {0,  1,  2,  1,  3,  2,  4,  5,  6,  5,  7,  6,  8,  9,  10, 8,  10, 11,
                 12, 13, 14, 12, 14, 15, 16, 17, 19, 17, 18, 19, 20, 23, 21, 21, 23, 22};

    // インデックスバッファ生成
    m_indexBuffer.Create(m_indices);

    // シェーダオブジェクト生成
    m_shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    bool sts = m_texture.Load("assets/texture/dice.png");
    assert(sts == true);

    m_material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f); // 真っ白
    mtrl.Ambient = Color(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.TextureEnable = true;
    m_material->Create(mtrl);

    float Scalesize = 0.3f;
    m_scale = Vector3(Scalesize);
    m_position = Vector3(-15.0f, 10.0f, 10.0f);
}

//=======================================
//更新処理
//=======================================
void TestCube::Update() {
    //m_Rotation.x += 0.01f;
    //m_Rotation.y += 0.01f;
    //m_Rotation.z += 0.01f;
}

//=======================================
//描画処理
//=======================================
void TestCube::Draw(Camera* cam) {
    //カメラを選択する
    cam->SetCamera();

    // SRT情報作成
    Matrix r = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
    Matrix t = Matrix::CreateTranslation(m_position.x, m_position.y, m_position.z);
    Matrix s = Matrix::CreateScale(m_scale.x, m_scale.y, m_scale.z);

    Matrix worldmtx;
    worldmtx = s * r * t;
    Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

    // 描画の処理
    ID3D11DeviceContext* devicecontext;
    devicecontext = Renderer::GetDeviceContext();

    // トポロジーをセット（プリミティブタイプ）
    devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //頂点の結び方の規則

    m_shader.SetGPU();
    m_vertexBuffer.SetGPU();
    m_indexBuffer.SetGPU();
    m_texture.SetGPU();

    if (m_material) {
        m_material->SetGPU();
    }

    devicecontext->DrawIndexed(m_indices.size(), // 描画するインデックス数
                               0, // 最初のインデックスバッファの位置
                               0);
}

//=======================================
//終了処理
//=======================================
void TestCube::Uninit() {}

void TestCube::SetMaterial(DirectX::SimpleMath::Color col) {
    for (auto& a : m_vertices) {
        a.color = col;
    }
    m_vertexBuffer.Create(m_vertices);
}

void TestCube::SetTexture(std::string texture) {
    bool sts = m_texture.Load(texture);
    assert(sts == true);
}
