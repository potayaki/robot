/// @file GhostModel.cpp
/// @brief FBXの材質色を保ったGhostと半透明の支援線を描く。
#include "GhostModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using namespace DirectX::SimpleMath;

namespace {
    class GhostMesh : public Mesh {
    public:
        bool Load() {
            Assimp::Importer importer;
            const auto scene =
                importer.ReadFile("assets/model/Ghost/Ghost.fbx",
                                  aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                      aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals);
            if (!scene || scene->mNumMeshes == 0) {
                return false;
            }
            for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
                const auto mesh = scene->mMeshes[i];
                aiColor4D color(1, 1, 1, 1);
                scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                const auto base = static_cast<unsigned>(m_vertices.size());
                for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
                    VERTEX_3D vertex{};
                    const auto pos = mesh->mVertices[v];
                    const auto normal = mesh->mNormals[v];
                    vertex.position = Vector3(pos.x, pos.y, pos.z);
                    vertex.normal = Vector3(normal.x, normal.y, normal.z);
                    vertex.color = Color(color.r, color.g, color.b, 1);
                    vertex.uv = Vector2(0.5f, 0.5f);
                    m_vertices.push_back(vertex);
                }
                for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
                    for (unsigned j = 0; j < mesh->mFaces[f].mNumIndices; ++j) {
                        m_indices.push_back(base + mesh->mFaces[f].mIndices[j]);
                    }
                }
            }
            if (m_vertices.empty() || m_indices.empty()) {
                return false;
            }
            Vector3 low = m_vertices[0].position, high = low;
            for (const auto& vertex : m_vertices) {
                low = Vector3::Min(low, vertex.position);
                high = Vector3::Max(high, vertex.position);
            }
            if (high.y - low.y < 0.001f) {
                return false;
            }
            const auto center = (low + high) * 0.5f;
            const float scale = 32.0f / (high.y - low.y);
            for (auto& vertex : m_vertices) {
                vertex.position = (vertex.position - center) * scale;
            }
            return true;
        }
    };
} // namespace

std::shared_ptr<GhostModel> GhostModel::Acquire() {
    static std::weak_ptr<GhostModel> cached;
    auto result = cached.lock();
    if (!result) {
        result = std::make_shared<GhostModel>();
        if (!result->Init()) {
            return {};
        }
        cached = result;
    }
    return result;
}

bool GhostModel::Init() {
    GhostMesh mesh;
    if (!mesh.Load() || !m_white.Load("assets/texture/white.png")) {
        return false;
    }
    m_mesh.Init(mesh);
    m_indexCount = static_cast<unsigned>(mesh.GetIndices().size());
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
    m_spiritShader.Create("shader/unlitTextureVS.hlsl", "shader/ghostSpiritPS.hlsl");
    m_vertices.resize(12);
    m_buffer.Create(m_vertices);
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    return SUCCEEDED(
        Renderer::GetDevice()->CreateRasterizerState(&rasterizer, m_rasterizer.ReleaseAndGetAddressOf()));
}

void GhostModel::Draw(Camera* camera, Vector3 center, float yaw) {
    camera->SetCamera();
    // 元モデルは-Z方向が顔。プレイヤーへ向く角度は+Z基準。
    Matrix world = Matrix::CreateRotationY(yaw + DirectX::XM_PI) * Matrix::CreateTranslation(center);
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    m_shader.SetGPU();
    m_white.SetGPU();
    m_mesh.BeforeDraw();
    m_mesh.DrawSubset(m_indexCount, 0, 0);
}

void GhostModel::DrawBeam(Camera* cam, Vector3 start, Vector3 end, float width, Color color, bool spirit) {
    const Vector3 direction = end - start;
    if (direction.LengthSquared() < 0.000001f) {
        return;
    }

    // 始点・終点を動かさず、板の幅方向だけをカメラへ向ける。
    // 地上のスライムにも空中のドローンにも、同じ方法で線を引ける。
    cam->SetCamera();
    const Matrix inverseView = cam->GetViewMatrix().Invert();
    const Vector3 center = (start + end) * 0.5f;
    const Vector3 toCamera = inverseView.Translation() - center;
    Vector3 side = direction.Cross(toCamera);

    // 光線を真正面から見る場合は外積が0になるので、別の軸で幅方向を作る。
    if (side.LengthSquared() < 0.000001f) {
        side = direction.Cross(Vector3(0.0f, 1.0f, 0.0f));
    }
    if (side.LengthSquared() < 0.000001f) {
        side = direction.Cross(Vector3(1.0f, 0.0f, 0.0f));
    }
    side.Normalize();

    if (spirit) {
        // 霊弾は進行方向によらずカメラに正対した円として表示する。
        side = Vector3::TransformNormal(Vector3(1, 0, 0), inverseView);
        side.Normalize();
        Vector3 up = Vector3::TransformNormal(Vector3(0, 1, 0), inverseView);
        up.Normalize();
        start = center - up * width * 0.5f;
        end = center + up * width * 0.5f;
    }

    // 色付きの外側と細い芯で支援線を見やすくする。
    const auto setStrip = [&](int firstVertex, float width, const Color& color) {
        const Vector3 offset = side * (width * 0.5f);
        const Vector3 corners[4] = {start - offset, start + offset, end - offset, end + offset};
        const int indices[6] = {0, 1, 2, 2, 1, 3};
        const Vector2 uv[4] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

        for (int i = 0; i < 6; ++i) {
            VERTEX_3D& vertex = m_vertices[firstVertex + i];
            vertex.position = corners[indices[i]];
            vertex.color = color;
            vertex.uv = spirit ? uv[indices[i]] : Vector2(0.5f, 0.5f);
        }
    };

    setStrip(0, width, Color(color.x, color.y, color.z, 0.3f));
    setStrip(6, spirit ? width : width * 0.3f, Color(color.x, color.y, color.z, 0.95f));
    m_buffer.Modify(m_vertices);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // 描画状態を保存し、このレーザーの設定を後続のUIや他のオブジェクトに残さない。
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> previousRasterizer;
    float previousBlendFactor[4]{};
    UINT previousSampleMask = 0;
    UINT previousStencilReference = 0;
    D3D11_PRIMITIVE_TOPOLOGY previousTopology;

    context->OMGetBlendState(previousBlend.GetAddressOf(), previousBlendFactor, &previousSampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &previousStencilReference);
    context->RSGetState(previousRasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&previousTopology);

    Renderer::SetBlendState(BS_ADDITIVE);
    // このプロジェクトのfalse設定は、深度テストを残して深度の書き込みだけを止める。
    // 地形や敵に隠れる部分は隠しつつ、透明な光で他の描画を遮らないようにする。
    Renderer::SetDepthEnable(false);
    context->RSSetState(m_rasterizer.Get());

    Matrix world = Matrix::Identity; // 頂点にワールド座標を直接設定している。
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    (spirit ? m_spiritShader : m_shader).SetGPU();
    m_white.SetGPU();
    m_buffer.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(m_vertices.size()), 0);

    context->OMSetBlendState(previousBlend.Get(), previousBlendFactor, previousSampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), previousStencilReference);
    context->RSSetState(previousRasterizer.Get());
    context->IASetPrimitiveTopology(previousTopology);
}
