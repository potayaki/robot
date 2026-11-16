#include "TitleFigureShadows.h"
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace {
    constexpr UINT ShadowSize = 1024;
}

bool TitleFigureShadows::Init() {
    auto device = Renderer::GetDevice();
    m_view = DirectX::XMMatrixLookAtLH(Vector3(-15, 25, -17.5f), Vector3::Zero, Vector3::Up);
    m_projection = DirectX::XMMatrixOrthographicLH(27.0f, 27.0f, 1.0f, 60.0f);
    const Matrix transform = (m_view * m_projection).Transpose();
    D3D11_BUFFER_DESC buffer{};
    buffer.ByteWidth = sizeof(transform);
    buffer.Usage = D3D11_USAGE_IMMUTABLE;
    buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA data{&transform};
    if (FAILED(device->CreateBuffer(&buffer, &data, m_lightMatrix.GetAddressOf()))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = desc.Height = ShadowSize;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf()))) {
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC depth{};
    depth.Format = DXGI_FORMAT_D32_FLOAT;
    depth.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(device->CreateDepthStencilView(texture.Get(), &depth, m_depth.GetAddressOf()))) {
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = DXGI_FORMAT_R32_FLOAT;
    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    view.Texture2D.MipLevels = 1;
    if (FAILED(device->CreateShaderResourceView(texture.Get(), &view, m_map.GetAddressOf()))) {
        return false;
    }

    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler.BorderColor[0] = sampler.BorderColor[1] = sampler.BorderColor[2] = sampler.BorderColor[3] = 1;
    sampler.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    m_ready = SUCCEEDED(device->CreateSamplerState(&sampler, m_comparison.GetAddressOf()));
    return m_ready;
}

void TitleFigureShadows::Capture(const std::function<void()>& drawCasters) {
    if (!m_ready) {
        return;
    }
    auto context = Renderer::GetDeviceContext();
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depth;
    context->OMGetRenderTargets(1, target.GetAddressOf(), depth.GetAddressOf());
    D3D11_VIEWPORT previous{};
    UINT count = 1;
    context->RSGetViewports(&count, &previous);
    const D3D11_VIEWPORT lightViewport{0, 0, float(ShadowSize), float(ShadowSize), 0, 1};
    context->RSSetViewports(1, &lightViewport);
    context->OMSetRenderTargets(0, nullptr, m_depth.Get());
    context->ClearDepthStencilView(m_depth.Get(), D3D11_CLEAR_DEPTH, 1, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    Renderer::SetDepthEnable(true);
    Renderer::SetViewMatrix(&m_view);
    Renderer::SetProjectionMatrix(&m_projection);
    drawCasters();
    context->OMSetRenderTargets(1, target.GetAddressOf(), depth.Get());
    context->RSSetViewports(count, &previous);
}

void TitleFigureShadows::BindGround() const {
    auto context = Renderer::GetDeviceContext();
    ID3D11Buffer* matrix = m_ready ? m_lightMatrix.Get() : nullptr;
    context->PSSetConstantBuffers(7, 1, &matrix);
    context->PSSetShaderResources(5, 1, m_map.GetAddressOf());
    context->PSSetSamplers(1, 1, m_comparison.GetAddressOf());
}

void TitleFigureShadows::UnbindGround() const {
    auto context = Renderer::GetDeviceContext();
    ID3D11ShaderResourceView* map = nullptr;
    ID3D11Buffer* matrix = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    context->PSSetShaderResources(5, 1, &map);
    context->PSSetConstantBuffers(7, 1, &matrix);
    context->PSSetSamplers(1, 1, &sampler);
}
