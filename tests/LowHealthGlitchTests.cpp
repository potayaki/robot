/// @file LowHealthGlitchTests.cpp
/// @brief 動作と境界条件を検証する LowHealthGlitchTests の自動テスト。

// Standalone WARP render test: supplies the small Renderer API used by the effect.
#include "../DX22_01_plane/LowHealthGlitch.h"
#include "../DX22_01_plane/LowHealthGlitchStrength.h"
#include <cassert>
#include <iostream>
#include <vector>

ID3D11Device* Renderer::m_pDevice = nullptr;
ID3D11DeviceContext* Renderer::m_pDeviceContext = nullptr;
ID3D11BlendState* Renderer::m_pBlendState[MAX_BLENDSTATE] = {};

HRESULT Renderer::Init() {
    return D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                             &m_pDevice, nullptr, &m_pDeviceContext);
}

void Renderer::Uninit() {
    m_pDeviceContext->ClearState();
    m_pDeviceContext->Release();
    m_pDevice->Release();
}

void Renderer::SetDepthEnable(bool) {
    D3D11_DEPTH_STENCIL_DESC desc{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> state;
    assert(SUCCEEDED(m_pDevice->CreateDepthStencilState(&desc, &state)));
    m_pDeviceContext->OMSetDepthStencilState(state.Get(), 0);
}

bool Renderer::CreateConstantBuffer(unsigned size, ID3D11Buffer** buffer) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, buffer));
}

int main() {
    assert(LowHealthGlitchStrength(100, 100) == 0);
    assert(LowHealthGlitchStrength(50, 100) == 0);
    assert(LowHealthGlitchStrength(49, 100) > 0);
    assert(LowHealthGlitchStrength(25, 100) == 0.5f);
    assert(LowHealthGlitchStrength(0, 100) == 1);
    assert(LowHealthGlitchStrength(-10, 100) == 1);
    assert(LowHealthGlitchStrength(10, 0) == 0);
    assert(LowHealthGlitchStrength(50, 200) == 0.5f);
    assert(SUCCEEDED(Renderer::Init()));
    {
        LowHealthGlitch glitch;
        auto device = Renderer::GetDevice();
        auto context = Renderer::GetDeviceContext();
        // Reuse the effect across different render sizes and MSAA modes.
        for (UINT samples : {1u, 4u}) {
            for (UINT width : {32u, 48u}) {
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = width;
                desc.Height = 24;
                desc.MipLevels = desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc = {samples, 0};
                desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                Microsoft::WRL::ComPtr<ID3D11Texture2D> target, resolved, staging;
                Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
                assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)));
                assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
                desc.SampleDesc = {1, 0};
                desc.BindFlags = 0;
                assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &resolved)));
                desc.Usage = D3D11_USAGE_STAGING;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)));
                D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), 24, 0, 1};
                context->RSSetViewports(1, &viewport);
                const auto render = [&](float strength) {
                    context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
                    const float clear[] = {0.2f, 0.3f, 0.4f, 1};
                    context->ClearRenderTargetView(rtv.Get(), clear);
                    glitch.Draw(strength, 1.25f);
                    context->OMSetRenderTargets(0, nullptr, nullptr);
                    if (samples > 1) {
                        context->ResolveSubresource(resolved.Get(), 0, target.Get(), 0, desc.Format);
                    } else {
                        context->CopyResource(resolved.Get(), target.Get());
                    }
                    context->CopyResource(staging.Get(), resolved.Get());
                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    assert(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
                    std::vector<unsigned char> pixels;
                    for (UINT y = 0; y < 24; ++y) {
                        const auto row =
                            static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch;
                        pixels.insert(pixels.end(), row, row + width * 4);
                    }
                    context->Unmap(staging.Get(), 0);
                    return pixels;
                };
                auto clean = render(0), mild = render(0.25f), severe = render(1);
                assert(clean != mild && mild != severe);
                assert(render(0) == clean); // Recovery leaves no residual distortion.
                unsigned mildRed = 0, severeRed = 0;
                for (size_t i = 0; i < clean.size(); i += 4) {
                    mildRed += mild[i];
                    severeRed += severe[i];
                    assert(severe[i + 3] == 255);
                }
                assert(severeRed > mildRed);
            }
        }
    }
    Renderer::Uninit();
    std::cout << "HP thresholds, red intensity, recovery, resize and 1x/4x MSAA passed.\n";
}
