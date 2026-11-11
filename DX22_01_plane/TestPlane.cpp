/// @file TestPlane.cpp
/// @brief 平面モデルの頂点作成と描画。

#include "TestPlane.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void TestPlane::Init() {
    // 頂点データ
    m_vertices.resize(4);

    m_vertices[0].position = Vector3(-10, 0, 10);
    m_vertices[1].position = Vector3(10, 0, 10);
    m_vertices[2].position = Vector3(-10, 0, -10);
    m_vertices[3].position = Vector3(10, 0, -10);

    m_vertices[0].color = Color(1, 1, 1, 1);
    m_vertices[1].color = Color(1, 1, 1, 1);
    m_vertices[2].color = Color(1, 1, 1, 1);
    m_vertices[3].color = Color(1, 1, 1, 1);

    m_vertices[0].uv = Vector2(0, 0);
    m_vertices[1].uv = Vector2(1, 0);
    m_vertices[2].uv = Vector2(0, 1);
    m_vertices[3].uv = Vector2(1, 1);

    // 頂点バッファ生成
    m_vertexBuffer.Create(m_vertices);

    // インデックデータ
    m_indices.resize(4);

    m_indices[0] = 0;
    m_indices[1] = 1;
    m_indices[2] = 2;
    m_indices[3] = 3;

    // インデックスバッファ生成
    m_indexBuffer.Create(m_indices);

    // シェーダオブジェクト生成
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    bool sts = m_texture.Load("assets/texture/field.jpg");
    assert(sts == true);

    m_position.y = 10;
    m_scale.x = 3;
    m_scale.z = 3;
}

//=======================================
//更新処理
//=======================================
void TestPlane::Update() {}

//=======================================
//描画処理
//=======================================
void TestPlane::Draw(Camera* cam) {
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
    devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_shader.SetGPU();
    m_vertexBuffer.SetGPU();
    m_indexBuffer.SetGPU();
    m_texture.SetGPU();

    devicecontext->DrawIndexed(m_indices.size(), // 描画するインデックス数
                               0, // 最初のインデックスバッファの位置
                               0);
}

//=======================================
//終了処理
//=======================================
void TestPlane::Uninit() {}
