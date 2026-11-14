#include "CStagePrimitive.h"
#include "Renderer.h"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

using namespace DirectX::SimpleMath;

struct CStagePrimitive::RenderResources {
    Shader shader;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> backFacesCulled;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> doubleSided;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;

    bool Init() {
        shader.Create("shader/stagePrimitiveVS.hlsl", "shader/stagePrimitivePS.hlsl");
        D3D11_RASTERIZER_DESC raster{};
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_BACK;
        raster.DepthClipEnable = TRUE;
        raster.MultisampleEnable = TRUE;
        auto device = Renderer::GetDevice();
        if (FAILED(device->CreateRasterizerState(&raster, backFacesCulled.GetAddressOf()))) {
            return false;
        }
        raster.CullMode = D3D11_CULL_NONE;
        if (FAILED(device->CreateRasterizerState(&raster, doubleSided.GetAddressOf()))) {
            return false;
        }
        D3D11_SAMPLER_DESC sampling{};
        sampling.Filter = D3D11_FILTER_ANISOTROPIC;
        sampling.AddressU = sampling.AddressV = sampling.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampling.MaxAnisotropy = 8;
        sampling.MaxLOD = D3D11_FLOAT32_MAX;
        sampling.ComparisonFunc = D3D11_COMPARISON_NEVER;
        return SUCCEEDED(device->CreateSamplerState(&sampling, sampler.GetAddressOf()));
    }
};

std::weak_ptr<CStagePrimitive::RenderResources> CStagePrimitive::s_sharedResources;

namespace {
    // stagePrimitiveCommon.hlsliのb6と同じ配置。行列はHLSLへの転置済みデータ。
    struct alignas(16) PrimitiveConstants {
        Matrix normalWorld;
        Color tint;
        Vector4 uvTransform;
        unsigned useTexture = 0;
        float padding[3]{};
    };

    static_assert(sizeof(PrimitiveConstants) == 112);

    bool Finite(const Vector3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }
} // namespace

void CStagePrimitive::Init() {
    if (m_resources) {
        return;
    }
    m_resources = s_sharedResources.lock();
    if (!m_resources) {
        auto resources = std::make_shared<RenderResources>();
        if (!resources->Init()) {
            return;
        }
        m_resources = resources;
        s_sharedResources = resources;
    }
    if (!Renderer::CreateConstantBuffer(sizeof(PrimitiveConstants), m_constants.GetAddressOf())) {
        m_resources.reset();
        return;
    }
    RebuildMesh();
}

bool CStagePrimitive::SetShape(PrimitiveMesh::Shape shape, int slices, int stacks) {
    if (shape < PrimitiveMesh::Shape::Triangle || shape >= PrimitiveMesh::Shape::Count) {
        return false;
    }
    const bool changed = shape != m_shape;
    m_shape = shape;
    m_slices = std::clamp(slices, 3, 128);
    m_stacks = std::clamp(stacks, 2, 64);
    if (changed) {
        m_doubleSided = PrimitiveMesh::IsFlat(shape);
    }
    if (m_resources) {
        RebuildMesh();
    }
    return true;
}

void CStagePrimitive::RebuildMesh() {
    const auto mesh = PrimitiveMesh::Build(m_shape, m_slices, m_stacks);
    std::vector<VERTEX_3D> vertices;
    vertices.reserve(mesh.vertices.size());
    for (const auto& source : mesh.vertices) {
        VERTEX_3D vertex{};
        vertex.position = Vector3(source.position.x, source.position.y, source.position.z);
        vertex.normal = Vector3(source.normal.x, source.normal.y, source.normal.z);
        vertex.color = Color(1, 1, 1, 1);
        vertex.uv = Vector2(source.uv.x, source.uv.y);
        vertices.push_back(vertex);
    }
    // ComPtrの既存バッファを解放してから再生成する。形状を何度変更しても積み残さない。
    m_vertices = VertexBuffer<VERTEX_3D>{};
    m_indices = IndexBuffer{};
    m_vertices.Create(vertices);
    m_indices.Create(mesh.indices);
    m_indexCount = static_cast<unsigned>(mesh.indices.size());
}

bool CStagePrimitive::SetTexture(const std::string& path) {
    if (path.empty()) {
        ClearTexture();
        return true;
    }
    // 一時オブジェクトへ読み込み、画像が見つからなくても表示中の画像を失わない。
    Texture candidate;
    if (!candidate.Load(path)) {
        m_textureError = "Could not load texture: " + path;
        return false;
    }
    m_texture = std::move(candidate);
    m_texturePath = path;
    m_textureError.clear();
    return true;
}

void CStagePrimitive::ClearTexture() {
    m_texture = Texture{};
    m_texturePath.clear();
    m_textureError.clear();
}

void CStagePrimitive::SetTextureTiling(float u, float v) {
    if (std::isfinite(u) && std::isfinite(v)) {
        m_uvScale = Vector2(u, v);
    }
}

void CStagePrimitive::SetTextureOffset(float u, float v) {
    if (std::isfinite(u) && std::isfinite(v)) {
        m_uvOffset = Vector2(u, v);
    }
}

void CStagePrimitive::SetTint(const Color& color) {
    if (!std::isfinite(color.x) || !std::isfinite(color.y) || !std::isfinite(color.z)) {
        return;
    }
    m_tint = Color(std::clamp(color.x, 0.0f, 1.0f), std::clamp(color.y, 0.0f, 1.0f),
                   std::clamp(color.z, 0.0f, 1.0f), 1.0f);
}

void CStagePrimitive::SetRotationDegrees(float x, float y, float z) {
    if (!Finite(Vector3(x, y, z))) {
        return;
    }
    m_rotation = Vector3(DirectX::XMConvertToRadians(x), DirectX::XMConvertToRadians(y),
                         DirectX::XMConvertToRadians(z));
}

void CStagePrimitive::Draw(Camera* camera) {
    if (!camera || !m_resources || !m_constants || !m_indexCount || IsDead() || !GetActive()) {
        return;
    }
    if (!Finite(m_scale) || !Finite(m_position) || !Finite(m_rotation) || m_scale.x <= 0 || m_scale.y <= 0 ||
        m_scale.z <= 0) {
        return;
    }

    auto context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> previousRaster;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> previousSampler;
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    Microsoft::WRL::ComPtr<ID3D11Buffer> previousVSConstants, previousPSConstants;
    float blendFactor[4]{};
    UINT sampleMask = 0, stencilReference = 0;
    context->RSGetState(previousRaster.GetAddressOf());
    context->PSGetSamplers(0, 1, previousSampler.GetAddressOf());
    context->OMGetBlendState(previousBlend.GetAddressOf(), blendFactor, &sampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &stencilReference);
    context->VSGetConstantBuffers(6, 1, previousVSConstants.GetAddressOf());
    context->PSGetConstantBuffers(6, 1, previousPSConstants.GetAddressOf());

    camera->SetCamera();
    Matrix world = Matrix::CreateScale(m_scale) *
                   Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z) *
                   Matrix::CreateTranslation(m_position);
    Renderer::SetWorldMatrix(&world);
    PrimitiveConstants constants{};
    // 法線は逆転置行列で変換する。それをGPU送信用に再び転置するとworldの逆行列になる。
    constants.normalWorld = world.Invert();
    constants.tint = m_tint;
    constants.uvTransform = Vector4(m_uvScale.x, m_uvScale.y, m_uvOffset.x, m_uvOffset.y);
    constants.useTexture = m_texture.GetShaderResourceView() ? 1 : 0;
    context->UpdateSubresource(m_constants.Get(), 0, nullptr, &constants, 0, 0);
    context->VSSetConstantBuffers(6, 1, m_constants.GetAddressOf());
    context->PSSetConstantBuffers(6, 1, m_constants.GetAddressOf());
    context->RSSetState(m_doubleSided ? m_resources->doubleSided.Get() : m_resources->backFacesCulled.Get());
    context->PSSetSamplers(0, 1, m_resources->sampler.GetAddressOf());
    Renderer::SetBlendState(BS_NONE);
    Renderer::SetDepthEnable(true);
    m_resources->shader.SetGPU();
    m_texture.SetGPU();
    m_vertices.SetGPU();
    m_indices.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_indexCount, 0, 0);

    context->RSSetState(previousRaster.Get());
    context->PSSetSamplers(0, 1, previousSampler.GetAddressOf());
    context->OMSetBlendState(previousBlend.Get(), blendFactor, sampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), stencilReference);
    context->VSSetConstantBuffers(6, 1, previousVSConstants.GetAddressOf());
    context->PSSetConstantBuffers(6, 1, previousPSConstants.GetAddressOf());
}

void CStagePrimitive::Uninit() {
    m_indexCount = 0;
    m_vertices = VertexBuffer<VERTEX_3D>{};
    m_indices = IndexBuffer{};
    m_constants.Reset();
    m_resources.reset();
    ClearTexture();
}

std::string CStagePrimitive::PlacementCode() const {
    std::ostringstream code;
    code.imbue(std::locale::classic());
    code << std::fixed << std::setprecision(6);
    code << "{\n    auto part = Game::GetInstance()->AddObject<CStagePrimitive>();\n";
    code << "    part->SetShape(PrimitiveMesh::Shape::" << PrimitiveMesh::Name(m_shape) << ", " << m_slices
         << ", " << m_stacks << ");\n";
    code << "    part->SetPosition(" << m_position.x << "f, " << m_position.y << "f, " << m_position.z
         << "f);\n";
    code << "    part->SetScale(" << m_scale.x << "f, " << m_scale.y << "f, " << m_scale.z << "f);\n";
    code << "    part->SetRotationDegrees(" << DirectX::XMConvertToDegrees(m_rotation.x) << "f, "
         << DirectX::XMConvertToDegrees(m_rotation.y) << "f, " << DirectX::XMConvertToDegrees(m_rotation.z)
         << "f);\n";
    if (!m_texturePath.empty()) {
        code << "    part->SetTexture(" << std::quoted(m_texturePath) << ");\n";
    }
    code << "    part->SetTextureTiling(" << m_uvScale.x << "f, " << m_uvScale.y << "f);\n";
    code << "    part->SetTextureOffset(" << m_uvOffset.x << "f, " << m_uvOffset.y << "f);\n";
    code << "    part->SetTint(DirectX::SimpleMath::Color(" << m_tint.x << "f, " << m_tint.y << "f, "
         << m_tint.z << "f, 1.0f));\n";
    code << "    part->SetDoubleSided(" << (m_doubleSided ? "true" : "false") << ");\n}\n";
    return code.str();
}
