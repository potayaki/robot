/// @file Arrow.cpp
/// @brief ゴルフ操作で方向と強さを示す矢印モデル。

#include "Arrow.h"
#include "Collision.h"
#include "Game.h"
#include "Golfball.h"

using namespace std;
using namespace DirectX::SimpleMath;

//=======================================
// 初期化処理
//=======================================
void Arrow::Init() {
    // メッシュ読み込み
    StaticMesh staticmesh;

    // 3Dモデルデータ
    std::u8string modelFile = u8"assets/model/arrow/arrow.fbx";

    // テクスチャディレクトリ
    std::string texDirectory = "assets/model/arrow";

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

    m_state = 1;
}

//=======================================
// 更新処理
//=======================================
void Arrow::Update() {
    if (m_state == 0) {
        return; // 非表示ならreturn
    }

    // ゴルフボールの位置を取得
    vector<GolfBall*> ballpt = Game::GetInstance()->GetObjects<GolfBall>();
    if (ballpt.size() > 0) {
        // 矢印の位置を更新
        m_position = ballpt[0]->GetPosition();
    }

    // 方向選択なら
    if (m_state == 1) {
        m_scale.z = 3; // 長さを固定

        // 向きを回転させる
        m_rotation.y += 0.03f;

        if (m_rotation.y > 6.28) {
            m_rotation.y = 0;
        }
    }
    // パワー選択なら
    else if (m_state == 2) {
        // 大きさを変更させる
        m_scale.z += 0.04f;
        if (m_scale.z > 4) {
            m_scale.z = 1;
        }
    }
}

//=======================================
// 描画処理
//=======================================
void Arrow::Draw(Camera* cam) {
    if (m_state == 0) {
        return; // 非表示ならreturn
    }

    //カメラを選択する
    cam->SetCamera();

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
void Arrow::Uninit() {}

//状態の設定
void Arrow::SetState(int s) {
    m_state = s;
}

// 矢印のベクトルを取得
Vector3 Arrow::GetVector() {
    //矢印の初期状態の向き
    Vector3 res = {0, 0, -1};

    // ベクトルを回転
    Matrix r = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
    res = Vector3::Transform(res, r);

    //矢印の長さ(パワー)を掛ける
    res *= m_scale.z;

    return res;
}
