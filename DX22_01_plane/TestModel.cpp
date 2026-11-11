/// @file TestModel.cpp
/// @brief ゲーム内で使用する静的モデルの読込とサブセット描画。

#include "TestModel.h"

#include <cmath>
#include <map>
#include <tuple>

using namespace DirectX::SimpleMath;

namespace {
    class SmoothStaticMesh : public StaticMesh {
    public:
        void SmoothNormals(float maxAngleDegrees) {
            using Position = std::tuple<float, float, float>;
            std::map<Position, std::vector<size_t>> verticesAtPosition;
            std::vector<Vector3> originalNormals;
            originalNormals.reserve(m_vertices.size());

            for (size_t i = 0; i < m_vertices.size(); ++i) {
                const auto& vertex = m_vertices[i];
                verticesAtPosition[{vertex.position.x, vertex.position.y, vertex.position.z}].push_back(i);
                originalNormals.push_back(vertex.normal);
            }

            const float minDot = std::cos(DirectX::XMConvertToRadians(maxAngleDegrees));
            for (const auto& [position, indices] : verticesAtPosition) {
                for (size_t i : indices) {
                    Vector3 reference = originalNormals[i];
                    if (reference.LengthSquared() < 0.000001f) {
                        continue;
                    }
                    reference.Normalize();

                    Vector3 average(0.0f, 0.0f, 0.0f);
                    for (size_t j : indices) {
                        Vector3 normal = originalNormals[j];
                        if (normal.LengthSquared() < 0.000001f) {
                            continue;
                        }
                        normal.Normalize();
                        if (reference.Dot(normal) >= minDot) {
                            average += normal;
                        }
                    }
                    if (average.LengthSquared() > 0.000001f) {
                        average.Normalize();
                        m_vertices[i].normal = average;
                    }
                }
            }
        }
    };
} // namespace

//=======================================
//初期化処理
//=======================================
void TestModel::Init() {
    // シェーダオブジェクト生成
    m_shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    m_scale = Vector3(1.0f, 1.0f, 1.0f);
    m_position = Vector3(0.0f, 0.0f, 0.0f);
    m_rotation = Vector3(0.0f, 0.0f, 0.0f);
}

//=======================================
//更新処理
//=======================================
void TestModel::Update() {}

//=======================================
//描画処理
//=======================================
void TestModel::Draw(Camera* cam) {
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
        // マテリアルをセット(サブセット情報の中にあるマテリアルインデックを使用)
        const auto materialIndex = m_subsets[i].MaterialIdx;
        m_materiales[materialIndex]->SetGPU();

        if (m_materiales[materialIndex]->isTextureEnable()) {
            m_textures[materialIndex]->SetGPU();
        }

        m_meshRenderer.DrawSubset(m_subsets[i].IndexNum, // 描画するインデックス数
                                  m_subsets[i].IndexBase, // 最初のインデックスバッファの位置
                                  m_subsets[i].VertexBase); // 頂点バッファの最初から使用
    }
}

void TestModel::Load(std::string modelFile, std::string texDirectory, std::string overrideTextureFile,
                     bool smoothNormals) {
    // メッシュ読み込み
    SmoothStaticMesh staticmesh;

    // 引数でもらったファイル名とフォルダ名を使ってMeshを読み込む
    staticmesh.Load(modelFile, texDirectory);
    if (smoothNormals) {
        // 同じ位置の頂点の法線を平均し、輪郭や UV は変えずに面の陰影だけを滑らかにする。
        staticmesh.SmoothNormals(80.0f);
    }
    m_meshRenderer.Init(staticmesh);

    // サブセット情報・テクスチャ情報取得
    m_subsets = staticmesh.GetSubsets();
    m_textures = staticmesh.GetTextures();

    // マテリアル情報取得
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();
    m_textures.resize(materials.size());

    // FBX が参照する元画像が手元にないモデルには、指定された画像を割り当てる。
    for (size_t i = 0; i < materials.size(); ++i) {
        if (!overrideTextureFile.empty()) {
            auto texture = std::make_unique<Texture>();
            if (texture->Load(overrideTextureFile)) {
                m_textures[i] = std::move(texture);
            }
        }

        // FBX に画像名があっても読み込みに失敗すると nullptr のままなので、
        // 実際に読み込めた画像がある場合にだけテクスチャを有効にする。
        materials[i].TextureEnable = i < m_textures.size() && m_textures[i] != nullptr;
    }

    // 念のため配列をクリアしておく
    m_materiales.clear();

    // マテリアル数分ループ
    for (int i = 0; i < materials.size(); i++) {
        std::unique_ptr<Material> m = std::make_unique<Material>();
        m->Create(materials[i]);
        m_materiales.push_back(std::move(m));
    }
}

//=======================================
//終了処理
//=======================================
void TestModel::Uninit() {}
