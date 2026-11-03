#include "EyeTurretModel.h"
#include "Renderer.h"

using namespace DirectX::SimpleMath;

void EyeTurretModel::Init()
{
    // FBXや外部の画像を使わず、滑らかな球と目専用のシェーダで白目・虹彩・瞳を描く。
    const auto mesh = EyeTurretShape::BuildSphere();
    std::vector<VERTEX_3D> vertices;
    vertices.reserve(mesh.positions.size());
    for (const Vector3& position : mesh.positions)
    {
        VERTEX_3D vertex{};
        vertex.position = position;
        vertex.normal = position;
        vertex.color = Color(1, 1, 1, 1);
        vertices.push_back(vertex);
    }
    m_vertices.Create(vertices);
    m_indices.Create(mesh.indices);
    m_indexCount = static_cast<unsigned int>(mesh.indices.size());
    m_shader.Create("shader/eyeTurretVS.hlsl", "shader/eyeTurretPS.hlsl");
}

void EyeTurretModel::Draw(Camera* camera, const Vector3& center, const Quaternion& rotation)
{

    if (camera == nullptr || m_indexCount == 0)
    {
        return;
    }
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    float previousBlendFactor[4]{};
    UINT previousSampleMask = 0;
    UINT previousStencilReference = 0;
    context->OMGetBlendState(previousBlend.GetAddressOf(), previousBlendFactor, &previousSampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &previousStencilReference);

    // 目は不透明な立体として描く。先に描いた火花などの加算合成を引き継がない。
    Renderer::SetBlendState(BS_NONE);
    Renderer::SetDepthEnable(true);
    camera->SetCamera();
    Matrix world = EyeTurretShape::WorldMatrix(center, rotation);
    Renderer::SetWorldMatrix(&world);
    m_shader.SetGPU();
    m_vertices.SetGPU();
    m_indices.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_indexCount, 0, 0);

    context->OMSetBlendState(previousBlend.Get(), previousBlendFactor, previousSampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), previousStencilReference);
}

void EyeTurretModel::Uninit()
{
    // プール終了時とデストラクタから重ねて呼ばれても安全に解放する。
    m_indexCount = 0;
    m_vertices = VertexBuffer<VERTEX_3D>{};
    m_indices = IndexBuffer{};
    m_shader = Shader{};
}
