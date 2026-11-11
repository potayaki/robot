/// @file Ground.cpp
/// @brief 地形グリッドの作成とテクスチャ付きの地面描画。

#include "Ground.h"
#include "stb_image.h"
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Ground::Init() {
    // 4つの頂点で1枚の平らな床を作る
    m_vertices.resize(4);

    m_vertices[0].position = Vector3(-1.0f, 0.0f, 1.0f); // 左奥
    m_vertices[1].position = Vector3(1.0f, 0.0f, 1.0f); // 右奥
    m_vertices[2].position = Vector3(-1.0f, 0.0f, -1.0f); // 手前左
    m_vertices[3].position = Vector3(1.0f, 0.0f, -1.0f); // 手前右

    for (int i = 0; i < 4; i++) {
        m_vertices[i].color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        m_vertices[i].normal = Vector3(0.0f, 1.0f, 0.0f); // 真上を向く法線
    }

    // UV座標（床が広いのでテクスチャを10回リピートさせる設定）
    m_vertices[0].uv = Vector2(0.0f, 0.0f);
    m_vertices[1].uv = Vector2(10.0f, 0.0f);
    m_vertices[2].uv = Vector2(0.0f, 10.0f);
    m_vertices[3].uv = Vector2(10.0f, 10.0f);

    // インデックスデータ (三角形2枚)
    m_indices.resize(6);
    m_indices[0] = 0;
    m_indices[1] = 1;
    m_indices[2] = 2;
    m_indices[3] = 2;
    m_indices[4] = 1;
    m_indices[5] = 3;

    // バッファ生成
    m_vertexBuffer.Create(m_vertices);
    m_indexBuffer.Create(m_indices);

    // シェーダオブジェクト生成
    // 時間帯に応じた照明に加え、遠景を空の地平線になじませる霞を描く。
    m_shader.Create("shader/groundSkyVS.hlsl", "shader/groundSkyPS.hlsl");

    // テクスチャの読み込み（画像パスは環境に合わせてください）
    bool sts = m_texture.Load("assets/texture/field.jpg");
    assert(sts == true);

    // マテリアルの設定
    m_material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    mtrl.TextureEnable = true;
    m_material->Create(mtrl);

    // 初期位置とスケール
    m_position = Vector3(0.0f, 0.0f, 0.0f);
    m_scale = Vector3(10000.0f, 0.0f, 10000.0f);
}

//=======================================
//更新処理
//=======================================
void Ground::Update() {}

//=======================================
//描画処理
//=======================================
void Ground::Draw(Camera* cam) {
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
    devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_shader.SetGPU();
    m_vertexBuffer.SetGPU();
    m_indexBuffer.SetGPU();
    m_texture.SetGPU();
    m_material->SetGPU();

    devicecontext->DrawIndexed(m_indices.size(), // 描画するインデックス数
                               0, // 最初のインデックスバッファの位置
                               0);
}

//=======================================
//終了処理
//=======================================
void Ground::Uninit() {}

std::vector<VERTEX_3D> Ground::GetVertices() {
    std::vector<VERTEX_3D> res;
    res.resize(m_vertices.size());

    Matrix r = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);

    Matrix t = Matrix::CreateTranslation(m_position.x, m_position.y, m_position.z);

    Matrix s = Matrix::CreateScale(m_scale.x, m_scale.y, m_scale.z);

    Matrix worldmtx = s * r * t;

    for (int i = 0; i < m_vertices.size(); i++) {
        res[i].position = Vector3::Transform(m_vertices[i].position, worldmtx);

        res[i].normal = Vector3::Transform(m_vertices[i].normal, worldmtx);

        res[i].color = m_vertices[i].color;
        res[i].uv = m_vertices[i].uv;
    }

    return res;
}
