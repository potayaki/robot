#include "StageSunShadows.h"
#include <cmath>

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace {
    // 半径10の機体から飛び散る破片を収め、地面を0.5ワールド単位刻みで比較する。
    constexpr UINT MapSize = 512;
    constexpr float MapWidth = 256.0f;
    constexpr float TexelWidth = MapWidth / MapSize;
    // 朝夕の低い光でも、空中の機体から離れた地面まで光線を届かせる。
    constexpr float LightDistance = 2048.0f;
    constexpr float DepthBias = 0.25f / (2 * LightDistance);
    // 森の風は b10、霧は b8 を使用する。
    constexpr UINT ConstantSlot = 11;
    constexpr UINT TextureSlot = 5;
    constexpr UINT SamplerSlot = 1;

    struct ShadowConstants {
        Matrix lightViewProjection;
        Vector4 parameters; // 逆解像度、正規化深度バイアス、有効フラグ、予備。
    };
} // namespace

bool StageSunShadows::Init() {
    *this = StageSunShadows{};
    auto device = Renderer::GetDevice();
    // マップ作成に失敗した場合も有効フラグはゼロのままで、通常の地面を描ける。
    const ShadowConstants disabled{Matrix::Identity, Vector4::Zero};
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = sizeof(ShadowConstants);
    buffer.Usage = D3D11_USAGE_DEFAULT;
    buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA initial{&disabled};
    if (FAILED(device->CreateBuffer(&buffer, &initial, m_constants.GetAddressOf()))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = textureDesc.Height = MapSize;
    textureDesc.MipLevels = textureDesc.ArraySize = textureDesc.SampleDesc.Count = 1;
    textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf()))) {
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC depth{};
    depth.Format = DXGI_FORMAT_D32_FLOAT;
    depth.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(device->CreateDepthStencilView(texture.Get(), &depth, m_depth.GetAddressOf()))) {
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC resource{};
    resource.Format = DXGI_FORMAT_R32_FLOAT;
    resource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resource.Texture2D.MipLevels = 1;
    if (FAILED(device->CreateShaderResourceView(texture.Get(), &resource, m_map.GetAddressOf()))) {
        return false;
    }
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sampler.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    for (auto& channel : sampler.BorderColor) {
        channel = 1.0f;
    }
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sampler, m_comparison.GetAddressOf()))) {
        return false;
    }
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    return SUCCEEDED(device->CreateRasterizerState(&rasterizer, m_rasterizer.GetAddressOf()));
}

void StageSunShadows::Capture(const Vector3& center, const Vector3& lightDirection,
                              const std::function<void()>& drawCaster) {
    if (!m_rasterizer) {
        return;
    }
    auto context = Renderer::GetDeviceContext();
    if (lightDirection.LengthSquared() < 0.000001f) {
        const ShadowConstants disabled{Matrix::Identity, Vector4::Zero};
        context->UpdateSubresource(m_constants.Get(), 0, nullptr, &disabled, 0, 0);
        return;
    }
    Vector3 direction = lightDirection;
    direction.Normalize();
    const Vector3 up = std::abs(direction.y) > 0.99f ? Vector3::UnitZ : Vector3::Up;
    Matrix view = DirectX::XMMatrixLookToLH(center - direction * LightDistance, direction, up);
    // 太陽視点でテクセル境界に合わせ、プレイヤーの微小な移動で輪郭がちらつくのを抑える。
    view._41 = std::round(view._41 / TexelWidth) * TexelWidth;
    view._42 = std::round(view._42 / TexelWidth) * TexelWidth;
    Matrix projection = DirectX::XMMatrixOrthographicLH(MapWidth, MapWidth, 0.0f, 2 * LightDistance);
    const ShadowConstants constants{(view * projection).Transpose(),
                                    Vector4(1.0f / MapSize, DepthBias, 1, 0)};
    context->UpdateSubresource(m_constants.Get(), 0, nullptr, &constants, 0, 0);

    ComPtr<ID3D11RenderTargetView> previousTarget;
    ComPtr<ID3D11DepthStencilView> previousDepth;
    context->OMGetRenderTargets(1, previousTarget.GetAddressOf(), previousDepth.GetAddressOf());
    ComPtr<ID3D11DepthStencilState> previousDepthState;
    UINT previousStencil = 0;
    context->OMGetDepthStencilState(previousDepthState.GetAddressOf(), &previousStencil);
    ComPtr<ID3D11RasterizerState> previousRasterizer;
    context->RSGetState(previousRasterizer.GetAddressOf());
    D3D11_VIEWPORT previousViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    UINT viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    context->RSGetViewports(&viewportCount, previousViewports);
    ComPtr<ID3D11PixelShader> previousPixelShader;
    context->PSGetShader(previousPixelShader.GetAddressOf(), nullptr, nullptr);

    ID3D11ShaderResourceView* noMap = nullptr;
    context->PSSetShaderResources(TextureSlot, 1, &noMap);
    context->OMSetRenderTargets(0, nullptr, m_depth.Get());
    context->ClearDepthStencilView(m_depth.Get(), D3D11_CLEAR_DEPTH, 1, 0);
    Renderer::SetDepthEnable(true);
    const D3D11_VIEWPORT viewport{0, 0, float(MapSize), float(MapSize), 0, 1};
    context->RSSetViewports(1, &viewport);
    context->RSSetState(m_rasterizer.Get());
    context->PSSetShader(nullptr, nullptr, 0);
    Renderer::SetViewMatrix(&view);
    Renderer::SetProjectionMatrix(&projection);
    drawCaster();

    context->OMSetRenderTargets(1, previousTarget.GetAddressOf(), previousDepth.Get());
    context->OMSetDepthStencilState(previousDepthState.Get(), previousStencil);
    context->RSSetViewports(viewportCount, previousViewports);
    context->RSSetState(previousRasterizer.Get());
    context->PSSetShader(previousPixelShader.Get(), nullptr, 0);
}

void StageSunShadows::BindGround() const {
    auto context = Renderer::GetDeviceContext();
    context->PSSetConstantBuffers(ConstantSlot, 1, m_constants.GetAddressOf());
    context->PSSetShaderResources(TextureSlot, 1, m_map.GetAddressOf());
    context->PSSetSamplers(SamplerSlot, 1, m_comparison.GetAddressOf());
}

void StageSunShadows::UnbindGround() const {
    auto context = Renderer::GetDeviceContext();
    ID3D11ShaderResourceView* noMap = nullptr;
    ID3D11Buffer* noConstants = nullptr;
    ID3D11SamplerState* noSampler = nullptr;
    context->PSSetShaderResources(TextureSlot, 1, &noMap);
    context->PSSetConstantBuffers(ConstantSlot, 1, &noConstants);
    context->PSSetSamplers(SamplerSlot, 1, &noSampler);
}
