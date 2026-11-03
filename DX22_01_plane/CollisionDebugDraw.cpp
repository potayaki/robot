#include "CollisionDebugDraw.h"
#ifdef _DEBUG
#include <cmath>

using namespace DirectX::SimpleMath;

void CollisionDebugDraw::Init()
{
    Uninit();
    constexpr unsigned segments = 64;
    std::vector<VERTEX_3D> vertices;
    vertices.reserve(3 * segments * 2);

    // XY・YZ・XZ平面の3つの円で球を示す。面の塗りつぶしや三角形の対角線は描かない。
    for (unsigned plane = 0; plane < 3; ++plane)
    {
        for (unsigned segment = 0; segment < segments; ++segment)
        {
            for (unsigned endpoint = 0; endpoint < 2; ++endpoint)
            {
                const float angle = DirectX::XM_2PI * (segment + endpoint) / segments;
                const float c = std::cos(angle);
                const float s = std::sin(angle);
                VERTEX_3D vertex{};
                if (plane == 0) vertex.position = Vector3(c, s, 0);
                else if (plane == 1) vertex.position = Vector3(0, c, s);
                else vertex.position = Vector3(c, 0, s);
                vertex.color = Color(1, 1, 1, 1);
                vertices.push_back(vertex);
            }
        }
    }
    m_vertices.Create(vertices);
    m_vertexCount = static_cast<unsigned>(vertices.size());
    m_shader.Create("shader/debugCollisionVS.hlsl", "shader/debugCollisionPS.hlsl");

    // モデルごとのマテリアルがb4を差し替えるため、線の色も専用バッファで明示的に設定する。
    if (!Renderer::CreateConstantBuffer(sizeof(MATERIAL), m_materialBuffer.GetAddressOf())) return;

    // 判定がモデルに埋まっていても読めるように重ねる。深度を書かず、HUDより先に描く。
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
    if (FAILED(Renderer::GetDevice()->CreateDepthStencilState(&depth, m_depth.GetAddressOf()))) return;

    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    if (FAILED(Renderer::GetDevice()->CreateRasterizerState(&rasterizer, m_rasterizer.GetAddressOf()))) return;
    m_ready = true;
}

void CollisionDebugDraw::Draw(Camera* camera, const std::vector<Sphere>& spheres)
{
    if (!m_ready || !camera || spheres.empty()) return;
    auto context = Renderer::GetDeviceContext();

    // 線用の設定を後続のミニマップやUIへ残さないよう、変更する描画状態を保存する。
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> previousRasterizer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> previousMaterial;
    float blendFactor[4]{};
    UINT sampleMask = 0, stencilReference = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->OMGetBlendState(previousBlend.GetAddressOf(), blendFactor, &sampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &stencilReference);
    context->RSGetState(previousRasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&topology);
    context->VSGetConstantBuffers(4, 1, previousMaterial.GetAddressOf());

    camera->SetCamera();
    Renderer::SetBlendState(BS_ALPHABLEND);
    context->OMSetDepthStencilState(m_depth.Get(), 0);
    context->RSSetState(m_rasterizer.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    m_shader.SetGPU();
    m_vertices.SetGPU();
    context->VSSetConstantBuffers(4, 1, m_materialBuffer.GetAddressOf());

    for (const auto& sphere : spheres)
    {
        if (sphere.shape.radius <= 0.0f) continue;
        Matrix world = Matrix::CreateScale(sphere.shape.radius) * Matrix::CreateTranslation(sphere.shape.center);
        Renderer::SetWorldMatrix(&world);
        MATERIAL material{};
        material.Diffuse = sphere.color;
        material.TextureEnable = FALSE;
        context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, &material, 0, 0);
        context->Draw(m_vertexCount, 0);
    }

    context->OMSetBlendState(previousBlend.Get(), blendFactor, sampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), stencilReference);
    context->RSSetState(previousRasterizer.Get());
    context->IASetPrimitiveTopology(topology);
    context->VSSetConstantBuffers(4, 1, previousMaterial.GetAddressOf());
}

void CollisionDebugDraw::Uninit()
{
    m_ready = false;
    m_vertexCount = 0;
    m_shader = Shader{};
    m_vertices = VertexBuffer<VERTEX_3D>{};
    m_materialBuffer.Reset();
    m_depth.Reset();
    m_rasterizer.Reset();
}
#endif
