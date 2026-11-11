/// @file LowHealthGlitch.cpp
/// @brief 低HP時の画面ノイズ演出と、描画結果の取り込み。

#include "LowHealthGlitch.h"

namespace {
    /// 画面演出専用の定数・画像・サンプラーのスロット。対応HLSLのb7/t7/s7に合わせる。
    constexpr UINT GlitchShaderSlot = 7;
    /// 強度・時刻・画面幅・画面高さの4成分をGPUへ渡す。
    constexpr UINT GlitchConstantsBytes = sizeof(float) * 4;
    /// 頂点IDから全画面を覆う大きな三角形を生成する。
    constexpr UINT FullScreenVertexCount = 3;
} // namespace

bool LowHealthGlitch::Init() {
    auto device = Renderer::GetDevice();
    Ptr<ID3DBlob> vs, ps, errors;
    const auto compile = [&](const char* entry, const char* profile, ID3DBlob** blob) {
        return D3DCompileFromFile(L"shader/lowHealthGlitch.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                  entry, profile, 0, 0, blob, errors.ReleaseAndGetAddressOf());
    };
    if (FAILED(compile("VSMain", "vs_5_0", &vs)) || FAILED(compile("PSMain", "ps_5_0", &ps))) {
        return false;
    }
    if (FAILED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &m_vs)) ||
        FAILED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &m_ps))) {
        return false;
    }
    if (!Renderer::CreateConstantBuffer(GlitchConstantsBytes, m_constants.ReleaseAndGetAddressOf())) {
        return false;
    }
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sampler, &m_sampler))) {
        return false;
    }
    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID;
    raster.CullMode = D3D11_CULL_NONE;
    raster.DepthClipEnable = TRUE;
    return SUCCEEDED(device->CreateRasterizerState(&raster, &m_raster));
}

void LowHealthGlitch::Draw(float strength, float time) {
    if (strength <= 0) {
        return;
    }
    if (!m_raster && !Init()) {
        return;
    }
    auto device = Renderer::GetDevice();
    auto context = Renderer::GetDeviceContext();
    Ptr<ID3D11RenderTargetView> target;
    Ptr<ID3D11DepthStencilView> depth;
    context->OMGetRenderTargets(1, &target, &depth);
    if (!target) {
        return;
    }
    Ptr<ID3D11Resource> resource;
    target->GetResource(&resource);
    Ptr<ID3D11Texture2D> source;
    if (FAILED(resource.As(&source))) {
        return;
    }
    D3D11_TEXTURE2D_DESC desc{}, previous{};
    source->GetDesc(&desc);
    if (m_capture) {
        m_capture->GetDesc(&previous);
    }
    if (!m_capture || previous.Width != desc.Width || previous.Height != desc.Height ||
        previous.Format != desc.Format) {
        m_view.Reset();
        m_capture.Reset();
        auto captureDesc = desc;
        captureDesc.SampleDesc = {1, 0};
        captureDesc.Usage = D3D11_USAGE_DEFAULT;
        captureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        captureDesc.CPUAccessFlags = captureDesc.MiscFlags = 0;
        if (FAILED(device->CreateTexture2D(&captureDesc, nullptr, &m_capture))) {
            return;
        }
        if (FAILED(device->CreateShaderResourceView(m_capture.Get(), nullptr, &m_view))) {
            m_capture.Reset();
            return;
        }
    }
    context->OMSetRenderTargets(0, nullptr, nullptr);
    if (desc.SampleDesc.Count > 1) {
        context->ResolveSubresource(m_capture.Get(), 0, source.Get(), 0, desc.Format);
    } else {
        context->CopyResource(m_capture.Get(), source.Get());
    }
    context->OMSetRenderTargets(1, target.GetAddressOf(), depth.Get());

    // HUDと次フレームの描画へ影響を残さないよう、変更する描画設定を保存して最後に戻す。
    Ptr<ID3D11BlendState> blend;
    float factors[4];
    UINT mask;
    context->OMGetBlendState(&blend, factors, &mask);
    Ptr<ID3D11DepthStencilState> depthState;
    UINT stencil;
    context->OMGetDepthStencilState(&depthState, &stencil);
    Ptr<ID3D11RasterizerState> raster;
    context->RSGetState(&raster);
    Ptr<ID3D11InputLayout> layout;
    context->IAGetInputLayout(&layout);
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->IAGetPrimitiveTopology(&topology);
    Ptr<ID3D11VertexShader> vs;
    context->VSGetShader(&vs, nullptr, nullptr);
    Ptr<ID3D11PixelShader> ps;
    context->PSGetShader(&ps, nullptr, nullptr);
    Ptr<ID3D11Buffer> buffer;
    context->PSGetConstantBuffers(GlitchShaderSlot, 1, &buffer);
    Ptr<ID3D11ShaderResourceView> view;
    context->PSGetShaderResources(GlitchShaderSlot, 1, &view);
    Ptr<ID3D11SamplerState> sampler;
    context->PSGetSamplers(GlitchShaderSlot, 1, &sampler);

    const float data[4] = {strength, time, static_cast<float>(desc.Width), static_cast<float>(desc.Height)};
    context->UpdateSubresource(m_constants.Get(), 0, nullptr, data, 0, 0);
    Renderer::SetDepthEnable(false);
    Renderer::SetBlendState(BS_NONE);
    context->RSSetState(m_raster.Get());
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);
    context->PSSetConstantBuffers(GlitchShaderSlot, 1, m_constants.GetAddressOf());
    context->PSSetShaderResources(GlitchShaderSlot, 1, m_view.GetAddressOf());
    context->PSSetSamplers(GlitchShaderSlot, 1, m_sampler.GetAddressOf());
    context->Draw(FullScreenVertexCount, 0);

    context->PSSetShaderResources(GlitchShaderSlot, 1, view.GetAddressOf());
    context->PSSetSamplers(GlitchShaderSlot, 1, sampler.GetAddressOf());
    context->PSSetConstantBuffers(GlitchShaderSlot, 1, buffer.GetAddressOf());
    context->VSSetShader(vs.Get(), nullptr, 0);
    context->PSSetShader(ps.Get(), nullptr, 0);
    context->IASetInputLayout(layout.Get());
    context->IASetPrimitiveTopology(topology);
    context->RSSetState(raster.Get());
    context->OMSetDepthStencilState(depthState.Get(), stencil);
    context->OMSetBlendState(blend.Get(), factors, mask);
}
