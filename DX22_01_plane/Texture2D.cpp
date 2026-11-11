/// @file Texture2D.cpp
/// @brief 画面上へ配置するテクスチャ付き2Dポリゴン。

#include "Texture2D.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
// 初期化処理
//=======================================
void Texture2D::Init() {
    // 頂点データ
    m_vertices.resize(4);

    m_vertices[0].position = Vector3(-0.5f, 0.5f, 0);
    m_vertices[1].position = Vector3(0.5f, 0.5f, 0);
    m_vertices[2].position = Vector3(-0.5f, -0.5f, 0);
    m_vertices[3].position = Vector3(0.5f, -0.5f, 0);

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

    // インデックスバッファ生成
    m_indices.resize(4);

    m_indices[0] = 0;
    m_indices[1] = 1;
    m_indices[2] = 2;
    m_indices[3] = 3;

    // インデックスバッファ生成
    m_indexBuffer.Create(m_indices);

    // シェーダオブジェクト生成
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    // マテリアル情報取得
    m_material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = Color(1, 1, 1, 1);
    mtrl.TextureEnable = true; // テクスチャを使うか否かのフラグ
    m_material->Create(mtrl);
}

//=======================================
// 更新処理
//=======================================
void Texture2D::Update() {}

//=======================================
// 描画処理
//=======================================
void Texture2D::Draw(Camera* cam) {
    Renderer::SetDepthEnable(false);

    //カメラを選択する
    cam->SetCamera(1);

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
    m_material->SetGPU();

    // UVの設定を指定
    float u = m_numU - 1;
    float v = m_numV - 1;
    float uw = 1 / m_splitX;
    float vh = 1 / m_splitY;

    Renderer::SetUV(u, v, uw, vh);

    //Renderer::SetUV(u, v, uw, vh);

    devicecontext->DrawIndexed((UINT)m_indices.size(), // 描画するインデックス数
                               0, // 最初のインデックスバッファの位置
                               0);

    Renderer::SetDepthEnable(true);
}

//=======================================
// 終了処理
//=======================================
void Texture2D::Uninit() {}

// テクスチャを指定
void Texture2D::SetTexture(const char* imgname) {
    // テクスチャロード
    bool sts = m_texture.Load(imgname);
    assert(sts == true);
}

// 位置を指定
void Texture2D::SetPosition(const float& x, const float& y, const float& z) {
    Vector3 p = {x, y, z};
    SetPosition(p);
}

void Texture2D::SetPosition(const Vector3& pos) {
    m_position = pos;
}

// 角度を指定
void Texture2D::SetRotation(const float& x, const float& y, const float& z) {
    Vector3 r = {x, y, z};
    SetRotation(r);
}

void Texture2D::SetRotation(const Vector3& rot) {
    m_rotation = rot * 3.14f / 180; // deg→radに変換
}

// 大きさを指定
void Texture2D::SetScale(const float& x, const float& y, const float& z) {
    Vector3 s = {x, y, z};
    SetScale(s);
}

void Texture2D::SetScale(const Vector3& scl) {
    m_scale = scl;
}

// UV座標を指定
void Texture2D::SetUV(const float& nu, const float& nv, const float& sx, const float& sy) {
    m_numU = nu;
    m_numV = nv;
    m_splitX = sx;
    m_splitY = sy;
}
