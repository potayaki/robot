/// @file ForestModel.cpp
/// @brief Quaterniusの色とローポリの法線を保ち、共有メッシュで森を描画する。
#include "ForestModel.h"
#include "FbxImportSettings.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>

using namespace DirectX::SimpleMath;

namespace {
    class ForestMesh : public Mesh {
    public:
        bool Load(const char* path) {
            Assimp::Importer importer;
            ConfigureFbxImport(importer);
            const auto scene =
                importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                            aiProcess_PreTransformVertices | aiProcess_GenNormals);
            if (!scene || !scene->HasMeshes()) {
                std::cerr << "Forest model load failed: " << path << " " << importer.GetErrorString() << '\n';
                return false;
            }
            for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
                const auto mesh = scene->mMeshes[meshIndex];
                aiColor4D diffuse(1, 1, 1, 1);
                scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
                const auto baseVertex = static_cast<unsigned int>(m_vertices.size());
                for (unsigned int index = 0; index < mesh->mNumVertices; ++index) {
                    const auto position = mesh->mVertices[index];
                    const auto normal = mesh->HasNormals() ? mesh->mNormals[index] : aiVector3D(0, 1, 0);
                    const auto color =
                        mesh->HasVertexColors(0) ? mesh->mColors[0][index] : aiColor4D(1, 1, 1, 1);
                    VERTEX_3D vertex{};
                    vertex.position = Vector3(position.x, position.y, position.z);
                    vertex.normal = Vector3(normal.x, normal.y, normal.z);
                    // FBXの線形色を、UNORMの描画先や草地の頂点色と同じ表示色にそろえる。
                    const Color linear(diffuse.r * color.r, diffuse.g * color.g, diffuse.b * color.b,
                                       diffuse.a * color.a);
                    vertex.color = Color(DirectX::XMColorRGBToSRGB(linear));
                    if (mesh->HasTextureCoords(0)) {
                        vertex.uv =
                            Vector2(mesh->mTextureCoords[0][index].x, mesh->mTextureCoords[0][index].y);
                    }
                    m_vertices.push_back(vertex);
                }
                for (unsigned int index = 0; index < mesh->mNumFaces; ++index) {
                    const auto& face = mesh->mFaces[index];
                    if (face.mNumIndices == 3) {
                        for (unsigned int corner = 0; corner < 3; ++corner) {
                            m_indices.push_back(baseVertex + face.mIndices[corner]);
                        }
                    }
                }
            }
            if (m_vertices.empty() || m_indices.empty()) {
                return false;
            }
            Vector3 low = m_vertices.front().position;
            Vector3 high = low;
            for (const auto& vertex : m_vertices) {
                low = Vector3::Min(low, vertex.position);
                high = Vector3::Max(high, vertex.position);
            }
            const float height = high.y - low.y;
            if (height <= 0.0001f) {
                return false;
            }
            const Vector3 bottomCenter((low.x + high.x) * 0.5f, low.y, (low.z + high.z) * 0.5f);
            for (auto& vertex : m_vertices) {
                vertex.position = (vertex.position - bottomCenter) / height;
            }
            return true;
        }
    };
} // namespace

bool ForestModel::Load(const char* path, const char* texturePath, bool windAffected) {
    ForestMesh source;
    if (!source.Load(path)) {
        return false;
    }
    Texture texture{};
    if (texturePath && !texture.Load(texturePath)) {
        return false;
    }
    MATERIAL data{};
    data.Diffuse = Color(1, 1, 1, 1);
    data.TextureEnable = texturePath != nullptr;
    auto material = std::make_unique<Material>();
    if (!material->Create(data)) {
        return false;
    }
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> state;
    if (FAILED(Renderer::GetDevice()->CreateRasterizerState(&rasterizer, state.GetAddressOf()))) {
        return false;
    }
    m_mesh.Init(source);
    m_material = std::move(material);
    m_texture = std::move(texture);
    m_rasterizer = std::move(state);
    m_shader.Create(windAffected ? "shader/forestFoliageVS.hlsl" : "shader/groundSkyVS.hlsl",
                    "shader/forestSceneryPS.hlsl");
    m_indexCount = static_cast<unsigned int>(source.GetIndices().size());
    return true;
}

void ForestModel::Draw(Camera* camera, const Vector3& feet, float yaw, float height) {
    if (!m_material || m_indexCount == 0 || height <= 0.0f) {
        return;
    }
    camera->SetCamera();
    Matrix world =
        Matrix::CreateScale(height) * Matrix::CreateRotationY(yaw) * Matrix::CreateTranslation(feet);
    Renderer::SetWorldMatrix(&world);
    m_shader.SetGPU();
    m_material->SetGPU();
    if (m_material->isTextureEnable()) {
        m_texture.SetGPU();
    }
    auto context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> previous;
    context->RSGetState(previous.GetAddressOf());
    context->RSSetState(m_rasterizer.Get());
    m_mesh.BeforeDraw();
    m_mesh.DrawSubset(m_indexCount, 0, 0);
    context->RSSetState(previous.Get());
}
