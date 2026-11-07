#include "BoarModel.h"
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include <iostream>

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr size_t MaxBones = 64;
    struct SkinVertex
    {
        VERTEX_3D vertex;
        std::array<unsigned, 4> bones;
        std::array<float, 4> weights;
    };
    static_assert(sizeof(SkinVertex) == 80);

    Matrix ToDirectX(const aiMatrix4x4& m)
    {
        return Matrix(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
                      m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
    }

    bool CreateConstantBuffer(UINT bytes, ComPtr<ID3D11Buffer>& buffer)
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = bytes;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        return SUCCEEDED(Renderer::GetDevice()->CreateBuffer(&desc, nullptr, buffer.GetAddressOf()));
    }
}

struct BoarModel::Resources
{
    VertexBuffer<SkinVertex> vertices;
    Texture albedo;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> bones, material;
    unsigned vertexCount = 0;
    Vector3 baseCenter{};
    float bottom = 0;

    // 予測線も共有する。描く直前に頂点を入れ替えるため、Boarごとのバッファは不要。
    Shader telegraphShader;
    VertexBuffer<VERTEX_3D> telegraphBuffer;
    std::vector<VERTEX_3D> telegraphVertices;
    ComPtr<ID3D11RasterizerState> telegraphRasterizer;
};

bool BoarModel::Init()
{
    m_resources.reset();
    std::string error;
    if (!m_rig.Load("assets/model/Boar/Boar.glb", error))
    {
        std::cerr << "Boar: " << error << '\n';
        return false;
    }
    static std::weak_ptr<Resources> cached;
    if (auto resources = cached.lock())
    {
        m_resources = resources;
        return true;
    }
    if (m_rig.GetMatrices().size() > MaxBones) return false;
    auto resources = std::make_shared<Resources>();
    const auto& image = m_rig.GetAlbedo();
    if (!resources->albedo.LoadFromFemory(image.data(), static_cast<int>(image.size()))) return false;

    std::vector<SkinVertex> vertices;
    for (const auto& source : m_rig.GetVertices())
    {
        SkinVertex vertex{};
        vertex.vertex.position = Vector3(source.position.x, source.position.y, source.position.z);
        vertex.vertex.normal = Vector3(source.normal.x, source.normal.y, source.normal.z);
        vertex.vertex.uv = Vector2(source.uv.x, source.uv.y);
        vertex.vertex.color = Color(0.8f, 0.8f, 0.8f, 1); // GLBのbaseColorFactor。
        vertex.bones = source.bones;
        vertex.weights = source.weights;
        vertices.push_back(vertex);
    }
    resources->vertices.Create(vertices);
    resources->vertexCount = static_cast<unsigned>(vertices.size());
    const auto bounds = m_rig.GetBounds();
    resources->bottom = bounds.min.y;
    resources->baseCenter = Vector3((bounds.min.x + bounds.max.x) * 0.5f, 0, (bounds.min.z + bounds.max.z) * 0.5f);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    if (FAILED(Renderer::CreateVertexShader(resources->vertexShader.GetAddressOf(), resources->layout.GetAddressOf(),
        layout, 6, "shader/boarVS.hlsl"))) return false;
    if (FAILED(Renderer::CreatePixelShader(resources->pixelShader.GetAddressOf(), "shader/litTexturePS.hlsl"))) return false;
    if (!CreateConstantBuffer(sizeof(Matrix) * MaxBones, resources->bones) ||
        !CreateConstantBuffer(sizeof(MATERIAL), resources->material)) return false;
    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    material.TextureEnable = TRUE;
    Renderer::GetDeviceContext()->UpdateSubresource(resources->material.Get(), 0, nullptr, &material, 0, 0);

    resources->telegraphShader.Create("shader/chargeTelegraphVS.hlsl", "shader/chargeTelegraphPS.hlsl");
    resources->telegraphVertices.resize(15); // 太い帯6頂点、中心線6頂点、先端の矢印3頂点。
    resources->telegraphBuffer.Create(resources->telegraphVertices);
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    if (FAILED(Renderer::GetDevice()->CreateRasterizerState(&rasterizer, resources->telegraphRasterizer.GetAddressOf()))) return false;
    m_resources = resources;
    cached = resources;
    return true;
}

void BoarModel::Update(BoarMotion motion, float deltaSeconds, float progress)
{
    if (!m_resources) return;
    m_motion = motion;
    m_rig.Update(motion, deltaSeconds, progress);
}

void BoarModel::Draw(Camera* camera, const Vector3& feet, float yaw, float scale)
{
    if (!m_resources || !camera) return;
    camera->SetCamera();
    const auto& resources = *m_resources;

    // 元モデルは-Zが正面。足元を地面へ合わせ、ゲーム内の+Zへ向きを補正する。
    float bottom = resources.bottom;
    if (m_motion == BoarMotion::Death) bottom = (std::min)(bottom, m_rig.GetBounds().min.y);
    Matrix world = Matrix::CreateTranslation(-resources.baseCenter.x, -bottom, -resources.baseCenter.z) *
        Matrix::CreateScale(scale) * Matrix::CreateRotationY(yaw + DirectX::XM_PI) * Matrix::CreateTranslation(feet);
    Renderer::SetWorldMatrix(&world);

    std::array<Matrix, MaxBones> matrices;
    matrices.fill(Matrix::Identity);
    for (size_t i = 0; i < m_rig.GetMatrices().size(); ++i) matrices[i] = ToDirectX(m_rig.GetMatrices()[i]).Transpose();
    auto context = Renderer::GetDeviceContext();
    ComPtr<ID3D11Buffer> previousVertexMaterial, previousPixelMaterial, previousBones;
    context->VSGetConstantBuffers(4, 1, previousVertexMaterial.GetAddressOf());
    context->PSGetConstantBuffers(4, 1, previousPixelMaterial.GetAddressOf());
    context->VSGetConstantBuffers(6, 1, previousBones.GetAddressOf());
    context->UpdateSubresource(resources.bones.Get(), 0, nullptr, matrices.data(), 0, 0);
    context->VSSetConstantBuffers(6, 1, resources.bones.GetAddressOf());
    // 他の敵やUIの材質が残っていても、Boar自身のテクスチャを正しく表示する。
    context->VSSetConstantBuffers(4, 1, resources.material.GetAddressOf());
    context->PSSetConstantBuffers(4, 1, resources.material.GetAddressOf());
    context->VSSetShader(resources.vertexShader.Get(), nullptr, 0);
    context->PSSetShader(resources.pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(resources.layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_resources->albedo.SetGPU();
    m_resources->vertices.SetGPU();
    context->Draw(resources.vertexCount, 0);
    // Rendererの共通材質だけを更新する既存の描画にも、Boarの材質を残さない。
    context->VSSetConstantBuffers(4, 1, previousVertexMaterial.GetAddressOf());
    context->PSSetConstantBuffers(4, 1, previousPixelMaterial.GetAddressOf());
    context->VSSetConstantBuffers(6, 1, previousBones.GetAddressOf());
}

void BoarModel::DrawTelegraph(Camera* camera, Vector3 start, Vector3 end,
                              float groundY, float halfWidth, float progress)
{
    if (!m_resources || !camera) return;
    camera->SetCamera();
    start.y = end.y = groundY + 0.15f; // 地面と重なってちらつかないよう、少しだけ上へ出す。
    Vector3 direction = end - start;
    if (direction.LengthSquared() < 0.001f) return;
    direction.Normalize();
    const Vector3 side(direction.z, 0, -direction.x);
    auto& vertices = m_resources->telegraphVertices;
    auto put = [&](unsigned index, const Vector3& position, const Color& color)
    {
        vertices[index].position = position;
        vertices[index].color = color;
    };
    auto strip = [&](unsigned first, float width, const Color& color)
    {
        const Vector3 offset = side * width;
        const Vector3 corners[] = {start - offset, start + offset, end - offset, end + offset};
        const unsigned indices[] = {0, 1, 2, 2, 1, 3};
        for (unsigned i = 0; i < 6; ++i) put(first + i, corners[indices[i]], color);
    };
    const float alpha = 0.20f + 0.15f * progress;
    strip(0, halfWidth, Color(1.0f, 0.02f, 0.02f, alpha));
    strip(6, 1.2f, Color(1.0f, 0.08f, 0.04f, 0.65f));
    put(12, end, Color(1, 0.04f, 0.02f, 0.7f));
    put(13, end - direction * 20.0f - side * halfWidth, vertices[12].color);
    put(14, end - direction * 20.0f + side * halfWidth, vertices[12].color);
    m_resources->telegraphBuffer.Modify(vertices);

    // 半透明の予測線は不透明な地形・敵の後に描く。深度は書かず、描画状態は元に戻す。
    auto context = Renderer::GetDeviceContext();
    ComPtr<ID3D11BlendState> blend;
    ComPtr<ID3D11DepthStencilState> depth;
    ComPtr<ID3D11RasterizerState> rasterizer;
    float factor[4]{};
    UINT mask = 0, stencil = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->OMGetBlendState(blend.GetAddressOf(), factor, &mask);
    context->OMGetDepthStencilState(depth.GetAddressOf(), &stencil);
    context->RSGetState(rasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&topology);
    Renderer::SetBlendState(BS_ALPHABLEND);
    Renderer::SetDepthEnable(false);
    context->RSSetState(m_resources->telegraphRasterizer.Get());
    Matrix identity = Matrix::Identity;
    Renderer::SetWorldMatrix(&identity);
    m_resources->telegraphShader.SetGPU();
    m_resources->telegraphBuffer.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(vertices.size()), 0);
    context->OMSetBlendState(blend.Get(), factor, mask);
    context->OMSetDepthStencilState(depth.Get(), stencil);
    context->RSSetState(rasterizer.Get());
    context->IASetPrimitiveTopology(topology);
}
