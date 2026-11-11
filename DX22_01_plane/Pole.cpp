/// @file Pole.cpp
/// @brief ゴルフの目標を示すポールモデル。

#include "Pole.h"
#include "Collision.h"
#include "Game.h"
#include "Golfball.h"
#include "Ground.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
// 初期化処理
//=======================================
void Pole::Init() {
    // メッシュ読み込み
    StaticMesh staticmesh;

    // 3Dモデルデータ
    std::u8string modelFile = u8"assets/model/golf_pole/golf_pole_NO_HOLE.obj";

    // テクスチャディレクトリ
    std::string texDirectory = "assets/model/golf_pole";

    // Meshを読み込む
    std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    staticmesh.Load(tmpStr1, texDirectory);

    m_meshRenderer.Init(staticmesh);

    // シェーダオブジェクト生成
    m_shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    // サブセット情報取得
    m_subsets = staticmesh.GetSubsets();

    // テクスチャ情報取得
    m_textures = staticmesh.GetTextures();

    // マテリアル情報取得
    vector<MATERIAL> materials = staticmesh.GetMaterials();

    // マテリアル数分ループ
    for (int i = 0; i < materials.size(); i++) {
        // マテリアルオブジェクト生成
        std::unique_ptr<Material> m = std::make_unique<Material>();

        // マテリアル情報をセット
        m->Create(materials[i]);

        // マテリアルオブジェクトを配列に追加
        m_materials.push_back(std::move(m));
    }

    // モデルによってスケールを調整
    m_scale.x = 3;
    m_scale.y = 3;
    m_scale.z = 3;
}

//=======================================
// 更新処理
//=======================================
void Pole::Update() {}

//=======================================
// 描画処理
//=======================================
void Pole::Draw(Camera* cam) {
    // カメラの設定を指定
    cam->SetCamera(0);

    // SRT情報作成
    Matrix r = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
    Matrix t = Matrix::CreateTranslation(m_position.x, m_position.y, m_position.z);
    Matrix s = Matrix::CreateScale(m_scale.x, m_scale.y, m_scale.z);

    Matrix worldmtx;
    worldmtx = s * r * t;
    Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

    m_shader.SetGPU();

    // インデックスバッファ・頂点バッファをセット
    m_meshRenderer.BeforeDraw();

    //マテリアル数分ループ
    for (int i = 0; i < m_subsets.size(); i++) {
        // マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
        m_materials[m_subsets[i].MaterialIdx]->SetGPU();

        if (m_materials[m_subsets[i].MaterialIdx]->isTextureEnable()) {
            m_textures[m_subsets[i].MaterialIdx]->SetGPU();
        }

        m_meshRenderer.DrawSubset(m_subsets[i].IndexNum, // 描画するインデックス数
                                  m_subsets[i].IndexBase, // 最初のインデックスバッファの位置
                                  m_subsets[i].VertexBase); // 頂点バッファの最初から使用
    }
}

//=======================================
// 終了処理
//=======================================
void Pole::Uninit() {}

//位置の設定
void Pole::SetPosition(float x, float y, float z) {
    Vector3 p = {x, y, z};
    SetPosition(p);
}

void Pole::SetPosition(Vector3 pos) {
    m_position = pos;

    // Y座標を地形に合わせて変更

    // Groundの頂点データを取得
    vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    vector<VERTEX_3D> vertices;
    for (auto& g : grounds) // Groundオブジェクトの数ループ
    {
        vector<VERTEX_3D> vecs = g->GetVertices();
        for (auto& v : vecs) // 頂点の数ループ
        {
            vertices.emplace_back(v);
        }
    }

    // 線分とポリゴンの当たり判定
    for (int i = 0; i < vertices.size(); i += 3) {
        // 三角形ポリゴン
        Collision::Polygon collisionPolygon = {vertices[i + 0].position, vertices[i + 1].position,
                                               vertices[i + 2].position};

        Vector3 cp; // 接触点

        Collision::Line line = {m_position, Vector3(0, 1, 0)};
        if (Collision::CheckHit(line, collisionPolygon, cp)) {
            m_position.y = cp.y; // 接触点のちょっとだけ上をPoleのY座標とする
        }
    }
}
