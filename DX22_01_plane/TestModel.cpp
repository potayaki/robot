/// @file TestModel.cpp
/// @brief ゲーム内で使用する静的モデルの読込とサブセット描画。

#include "TestModel.h"
#include "EnemyTint.h"

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

struct TestModel::Resources {
    /// モデルの頂点・インデックスを保持する共有GPU資源。
    MeshRenderer m_mesh;
    /// サブセットのMaterialIdxに対応する共有材質。
    std::vector<std::unique_ptr<Material>> m_materials;
    /// 材質ごとの描画範囲。
    std::vector<SUBSET> m_subsets;
    /// 材質に対応する共有テクスチャ。画像なしの場合はnullptr。
    std::vector<std::unique_ptr<Texture>> m_textures;
    /// 個体色を描画時だけ適用する共有資源。
    std::shared_ptr<EnemyTint> m_tint;
};

std::shared_ptr<TestModel> TestModel::AcquireShared(std::weak_ptr<TestModel>& cachedModel,
                                                    const char* modelFile, const char* textureDirectory) {
    if (auto model = cachedModel.lock()) {
        return model;
    }

    auto model = std::make_shared<TestModel>();
    model->Init();
    model->Load(modelFile, textureDirectory);
    cachedModel = model;
    return model;
}

//=======================================
//初期化処理
//=======================================
void TestModel::Init() {
    m_resources.reset();
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
void TestModel::Draw(Camera* camera) {
    Draw(camera, m_position, m_rotation, m_scale);
}

void TestModel::Draw(Camera* camera, const Color& tint) {
    Draw(camera, m_position, m_rotation, m_scale, tint);
}

void TestModel::Draw(Camera* camera, const Vector3& worldPosition, const Vector3& rotationRadians,
                     const Vector3& modelScale, const Color& tint) {
    if (!m_resources || !camera) {
        return;
    }
    camera->SetCamera();

    // 姿勢は描画のたびに組み立てるため、同じメッシュを使う弾もそれぞれの位置・向きで描ける。
    const Matrix rotation =
        Matrix::CreateFromYawPitchRoll(rotationRadians.y, rotationRadians.x, rotationRadians.z);
    Matrix world = Matrix::CreateScale(modelScale) * rotation * Matrix::CreateTranslation(worldPosition);
    Renderer::SetWorldMatrix(&world);
    m_shader.SetGPU();
    EnemyTintScope tintScope(m_resources->m_tint, tint);
    m_resources->m_mesh.BeforeDraw();

    for (const auto& subset : m_resources->m_subsets) {
        Material& material = *m_resources->m_materials[subset.MaterialIdx];
        material.SetGPU();
        if (material.isTextureEnable()) {
            m_resources->m_textures[subset.MaterialIdx]->SetGPU();
        }
        m_resources->m_mesh.DrawSubset(subset.IndexNum, subset.IndexBase, subset.VertexBase);
    }
}

void TestModel::Load(std::string modelFile, std::string texDirectory, std::string overrideTextureFile,
                     bool smoothNormals) {
    // 群れの各個体でFBXの再解析・画像再読込を繰り返さない。姿勢はTestModel自身に保持する。
    using CacheKey = std::tuple<std::string, std::string, std::string, bool>;
    static std::map<CacheKey, std::weak_ptr<Resources>> cached;
    std::erase_if(cached, [](const auto& entry) { return entry.second.expired(); });
    const CacheKey key{modelFile, texDirectory, overrideTextureFile, smoothNormals};
    if (auto existing = cached[key].lock()) {
        m_resources = existing;
        return;
    }
    auto resources = std::make_shared<Resources>();
    resources->m_tint = EnemyTint::Acquire();
    // メッシュ読み込み
    SmoothStaticMesh staticmesh;

    // 引数でもらったファイル名とフォルダ名を使ってMeshを読み込む
    staticmesh.Load(modelFile, texDirectory);
    if (smoothNormals) {
        // 同じ位置の頂点の法線を平均し、輪郭や UV は変えずに面の陰影だけを滑らかにする。
        staticmesh.SmoothNormals(80.0f);
    }
    resources->m_mesh.Init(staticmesh);

    // サブセット情報・テクスチャ情報取得
    resources->m_subsets = staticmesh.GetSubsets();
    resources->m_textures = staticmesh.GetTextures();

    // マテリアル情報取得
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();
    resources->m_textures.resize(materials.size());

    // FBX が参照する元画像が手元にないモデルには、指定された画像を割り当てる。
    for (size_t i = 0; i < materials.size(); ++i) {
        if (!overrideTextureFile.empty()) {
            auto texture = std::make_unique<Texture>();
            if (texture->Load(overrideTextureFile)) {
                resources->m_textures[i] = std::move(texture);
            }
        }

        // FBX に画像名があっても読み込みに失敗すると nullptr のままなので、
        // 実際に読み込めた画像がある場合にだけテクスチャを有効にする。
        materials[i].TextureEnable = i < resources->m_textures.size() && resources->m_textures[i] != nullptr;
    }

    // 描画時にサブセットの MaterialIdx が使えるよう、元の材質順を保つ。
    for (const auto& material : materials) {
        resources->m_materials.push_back(std::make_unique<Material>(material));
    }
    m_resources = resources;
    cached[key] = resources;
}

//=======================================
//終了処理
//=======================================
void TestModel::Uninit() {
    m_resources.reset();
}
