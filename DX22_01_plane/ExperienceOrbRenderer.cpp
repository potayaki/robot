/// @file ExperienceOrbRenderer.cpp
/// @brief カメラに向けた板に経験値オーブを描く。揺れは描画位置にだけ加える。

#include "ExperienceOrbRenderer.h"
#include "Camera.h"
#include "ExperienceOrb.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    constexpr UINT BatchOrbs = 128;
    constexpr UINT VerticesPerOrb = 6;
    constexpr float GlowRadius = 3.4f; // 群れの中でも個々に見える周辺光の外半径（ワールド単位）。
    constexpr float BobHeight = 0.7f; // 見た目に加える上下動の振幅（ワールド単位）。
} // namespace

bool ExperienceOrbRenderer::Init() {
    Uninit();
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = BatchOrbs * VerticesPerOrb * sizeof(VERTEX_3D);
    buffer.Usage = D3D11_USAGE_DYNAMIC;
    buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    auto* device = Renderer::GetDevice();
    if (FAILED(device->CreateBuffer(&buffer, nullptr, m_vertices.GetAddressOf()))) {
        return false;
    }
    D3D11_BLEND_DESC blend{};
    auto& target = blend.RenderTarget[0];
    target.BlendEnable = TRUE;
    target.SrcBlend = target.SrcBlendAlpha = D3D11_BLEND_ONE;
    target.DestBlend = target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    target.BlendOp = target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device->CreateBlendState(&blend, m_blend.GetAddressOf()))) {
        Uninit();
        return false;
    }
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/experienceOrbPS.hlsl");
    return true;
}

void ExperienceOrbRenderer::Draw(Camera* camera, std::span<const ExperienceOrb> orbs) {
    if (!camera || orbs.empty() || !m_blend) {
        return;
    }
    camera->SetCamera();
    const Matrix billboard = camera->GetViewMatrix().Invert();
    const Vector3 right = billboard.Right() * GlowRadius;
    const Vector3 up = billboard.Up() * GlowRadius;
    auto* context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    float factors[4]{};
    UINT mask = 0, stencil = 0;
    D3D11_PRIMITIVE_TOPOLOGY previousTopology{};
    context->OMGetBlendState(previousBlend.GetAddressOf(), factors, &mask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &stencil);
    context->IAGetPrimitiveTopology(&previousTopology);
    context->OMSetBlendState(m_blend.Get(), nullptr, 0xffffffff);
    Renderer::SetDepthEnable(false); // 地形で遮蔽し、透明な光は深度に書き込まない。
    Matrix world = Matrix::Identity;
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    m_shader.SetGPU();
    UINT stride = sizeof(VERTEX_3D), offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertices.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    while (!orbs.empty()) {
        const auto count = (std::min)(size_t(BatchOrbs), orbs.size());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(m_vertices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            break;
        }
        auto* vertex = static_cast<VERTEX_3D*>(mapped.pData);
        for (const auto& orb : orbs.first(count)) {
            const float wave = std::sin(orb.phase);
            const Vector3 center = orb.position + Vector3::UnitY * (BobHeight * wave);
            const Color intensity(1, 1, 1, 0.85f + 0.15f * wave);
            const VERTEX_3D tl{center - right + up, {}, intensity, {0, 0}};
            const VERTEX_3D tr{center + right + up, {}, intensity, {1, 0}};
            const VERTEX_3D bl{center - right - up, {}, intensity, {0, 1}};
            const VERTEX_3D br{center + right - up, {}, intensity, {1, 1}};
            *vertex++ = tl;
            *vertex++ = tr;
            *vertex++ = bl;
            *vertex++ = bl;
            *vertex++ = tr;
            *vertex++ = br;
        }
        context->Unmap(m_vertices.Get(), 0);
        context->Draw(static_cast<UINT>(count) * VerticesPerOrb, 0);
        orbs = orbs.subspan(count);
    }
    context->OMSetBlendState(previousBlend.Get(), factors, mask);
    context->OMSetDepthStencilState(previousDepth.Get(), stencil);
    context->IASetPrimitiveTopology(previousTopology);
}

void ExperienceOrbRenderer::Uninit() {
    m_vertices.Reset();
    m_blend.Reset();
    m_shader = Shader{};
}
