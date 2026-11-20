/// @file MushroomModel.cpp
/// @brief キノコモデルの読込と、通常色・予告色の描画。

#include "MushroomModel.h"
#include "FbxImportSettings.h"
#include "EnemyTint.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

using namespace DirectX::SimpleMath;

namespace {
    class MushroomMesh : public Mesh {
    public:
        /// 材質ごとに分けた描画範囲と使用するインデックスの情報。
        std::vector<SUBSET> m_subsets;

        bool Load() {
            Assimp::Importer importer;
            ConfigureFbxImport(importer);
            // Replace the FBX's flat normals; GenSmoothNormals alone keeps existing normals.
            importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
            const auto scene = importer.ReadFile("assets/model/mushroom/Mushroom.fbx",
                                                 aiProcess_ConvertToLeftHanded | aiProcess_Triangulate |
                                                     aiProcess_PreTransformVertices |
                                                     aiProcess_GenSmoothNormals | aiProcess_ForceGenNormals);
            if (!scene) {
                return false;
            }
            for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
                const auto mesh = scene->mMeshes[i];
                SUBSET subset{};
                subset.VertexBase = static_cast<unsigned int>(m_vertices.size());
                subset.IndexBase = static_cast<unsigned int>(m_indices.size());
                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    VERTEX_3D vertex{};
                    const auto p = mesh->mVertices[v];
                    const auto n = mesh->mNormals[v];
                    // This FBX stands along -Z. Rotate +90 degrees around X
                    // before measuring its height and aligning its feet to Y=0.
                    vertex.position = Vector3(p.x, -p.z, p.y);
                    vertex.normal = Vector3(n.x, -n.z, n.y);
                    vertex.color = Color(1, 1, 1, 1);
                    if (mesh->HasTextureCoords(0)) {
                        vertex.uv = Vector2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
                    }
                    m_vertices.push_back(vertex);
                }
                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    for (unsigned int j = 0; j < mesh->mFaces[f].mNumIndices; ++j) {
                        m_indices.push_back(mesh->mFaces[f].mIndices[j]);
                    }
                }
                subset.IndexNum = static_cast<unsigned int>(m_indices.size()) - subset.IndexBase;
                m_subsets.push_back(subset);
            }
            return FitToGround(28.0f);
        }

        bool FitToGround(float height) {
            if (m_vertices.empty() || m_indices.empty()) {
                return false;
            }
            Vector3 low = m_vertices[0].position, high = low;
            for (const auto& v : m_vertices) {
                low = Vector3::Min(low, v.position);
                high = Vector3::Max(high, v.position);
            }
            if (high.y - low.y < 0.0001f) {
                return false;
            }
            const float scale = height / (high.y - low.y);
            const Vector3 origin((low.x + high.x) * 0.5f, low.y, (low.z + high.z) * 0.5f);
            for (auto& v : m_vertices) {
                v.position = (v.position - origin) * scale;
            }
            return true;
        }
    };
} // namespace

struct MushroomModel::Resources {
    /// キノコモデルの頂点とインデックス。
    MeshRenderer m_mesh;
    /// 通常の照明付き描画シェーダー。
    Shader m_normalShader;
    /// 個体色より優先する自爆警告の赤色シェーダー。
    Shader m_redShader;
    /// 材質ごとの描画範囲。
    std::vector<SUBSET> m_subsets;
    /// サブセットごとの材質。
    std::vector<std::unique_ptr<Material>> m_materials;
    /// 同梱のベースカラー画像。
    Texture m_texture;
    /// 個体色を描画時だけ適用する共有資源。
    std::shared_ptr<EnemyTint> m_tint;
};

bool MushroomModel::Init() {
    static std::weak_ptr<Resources> cached;
    if (auto resources = cached.lock()) {
        m_resources = resources;
        return true;
    }
    auto resources = std::make_shared<Resources>();
    MushroomMesh mesh;
    if (!mesh.Load()) {
        return false;
    }
    resources->m_mesh.Init(mesh);
    resources->m_subsets = mesh.m_subsets;
    const bool textured = resources->m_texture.Load("assets/model/mushroom/albedo.tga");
    {
        MATERIAL data{};
        data.Diffuse = Color(1, 1, 1, 1);
        data.TextureEnable = textured;
        auto material = std::make_unique<Material>();
        material->Create(data);
        resources->m_materials.push_back(std::move(material));
    }
    resources->m_normalShader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
    resources->m_redShader.Create("shader/litTextureVS.hlsl", "shader/mushroomWarningPS.hlsl");
    resources->m_tint = EnemyTint::Acquire();
    if (resources->m_materials.empty() || resources->m_subsets.empty()) {
        return false;
    }
    m_resources = resources;
    cached = resources;
    return true;
}

void MushroomModel::Draw(Camera* camera, Vector3 feet, float yaw, bool red, float scale, const Color& tint) {
    if (!m_resources || !camera) {
        return;
    }
    camera->SetCamera();
    // The upright model faces -Z; movement yaw uses +Z as its forward direction.
    Matrix world = Matrix::CreateScale(scale) * Matrix::CreateRotationY(yaw + DirectX::XM_PI) *
                   Matrix::CreateTranslation(feet);
    Renderer::SetWorldMatrix(&world);
    (red ? m_resources->m_redShader : m_resources->m_normalShader).SetGPU();
    // 赤点滅は亜種・希少種でも優先し、自爆の予備動作を識別できるようにする。
    EnemyTintScope tintScope(m_resources->m_tint, red ? Color(1, 1, 1, 0) : tint);
    m_resources->m_mesh.BeforeDraw();
    m_resources->m_texture.SetGPU();
    for (const auto& subset : m_resources->m_subsets) {
        m_resources->m_materials[subset.MaterialIdx]->SetGPU();
        m_resources->m_mesh.DrawSubset(subset.IndexNum, subset.IndexBase, subset.VertexBase);
    }
}
