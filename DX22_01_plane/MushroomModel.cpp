#include "MushroomModel.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

using namespace DirectX::SimpleMath;
namespace
{
    class MushroomMesh : public Mesh
    {
    public:
        std::vector<SUBSET> subsets;
        bool Load()
        {
            Assimp::Importer importer;
            // Replace the FBX's flat normals; GenSmoothNormals alone keeps existing normals.
            importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
            const auto scene = importer.ReadFile("assets/model/mushroom/Mushroom.fbx",
                aiProcess_ConvertToLeftHanded | aiProcess_Triangulate |
                aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_ForceGenNormals);
            if (!scene) return false;
            for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            {
                const auto mesh = scene->mMeshes[i];
                SUBSET subset{};
                subset.VertexBase = static_cast<unsigned int>(m_vertices.size());
                subset.IndexBase = static_cast<unsigned int>(m_indices.size());
                for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
                {
                    VERTEX_3D vertex{};
                    const auto p = mesh->mVertices[v];
                    const auto n = mesh->mNormals[v];
                    // This FBX stands along -Z. Rotate +90 degrees around X
                    // before measuring its height and aligning its feet to Y=0.
                    vertex.position = Vector3(p.x, -p.z, p.y);
                    vertex.normal = Vector3(n.x, -n.z, n.y);
                    vertex.color = Color(1, 1, 1, 1);
                    if (mesh->HasTextureCoords(0))
                        vertex.uv = Vector2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
                    m_vertices.push_back(vertex);
                }
                for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
                    for (unsigned int j = 0; j < mesh->mFaces[f].mNumIndices; ++j)
                        m_indices.push_back(mesh->mFaces[f].mIndices[j]);
                subset.IndexNum = static_cast<unsigned int>(m_indices.size()) - subset.IndexBase;
                subsets.push_back(subset);
            }
            return FitToGround(28.0f);
        }
        bool FitToGround(float height)
        {
            if (m_vertices.empty() || m_indices.empty()) return false;
            Vector3 low = m_vertices[0].position, high = low;
            for (const auto& v : m_vertices)
            {
                low = Vector3::Min(low, v.position);
                high = Vector3::Max(high, v.position);
            }
            if (high.y - low.y < 0.0001f) return false;
            const float scale = height / (high.y - low.y);
            const Vector3 origin((low.x + high.x) * 0.5f, low.y, (low.z + high.z) * 0.5f);
            for (auto& v : m_vertices) v.position = (v.position - origin) * scale;
            return true;
        }
    };
}

bool MushroomModel::Init()
{
    MushroomMesh mesh;
    if (!mesh.Load()) return false;
    m_mesh.Init(mesh);
    m_subsets = mesh.subsets;
    const bool textured = m_texture.Load("assets/model/mushroom/albedo.tga");
    {
        MATERIAL data{};
        data.Diffuse = Color(1, 1, 1, 1);
        data.TextureEnable = textured;
        auto material = std::make_unique<Material>();
        material->Create(data);
        m_materials.push_back(std::move(material));
    }
    m_normalShader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
    m_redShader.Create("shader/litTextureVS.hlsl", "shader/mushroomWarningPS.hlsl");
    return !m_materials.empty() && !m_subsets.empty();
}

void MushroomModel::Draw(Camera* camera, Vector3 feet, float yaw, bool red)
{
    camera->SetCamera();
    // The upright model faces -Z; movement yaw uses +Z as its forward direction.
    Matrix world = Matrix::CreateRotationY(yaw + DirectX::XM_PI) * Matrix::CreateTranslation(feet);
    Renderer::SetWorldMatrix(&world);
    (red ? m_redShader : m_normalShader).SetGPU();
    m_mesh.BeforeDraw();
    m_texture.SetGPU();
    for (const auto& subset : m_subsets)
    {
        m_materials[subset.MaterialIdx]->SetGPU();
        m_mesh.DrawSubset(subset.IndexNum, subset.IndexBase, subset.VertexBase);
    }
}
