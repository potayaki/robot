/// @file ExplosionRenderTests.cpp
/// @brief 実際の爆発画像と billboard 描画を WARP で描き、透明部分の遮蔽を検証する。

#include "../DX22_01_plane/billboard.h"
#include "../DX22_01_plane/Application.h"
#include "../DX22_01_plane/ExplosionManager.h"
#include <array>
#include <cassert>
#include <iostream>

#pragma comment(lib, "user32.lib")

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

// ウィンドウを作らず、実際の Shader・Texture・billboard を使うための描画環境。
ID3D11Device* Renderer::m_pDevice = nullptr;
ID3D11DeviceContext* Renderer::m_pDeviceContext = nullptr;
ID3D11BlendState* Renderer::m_pBlendState[MAX_BLENDSTATE] = {};
ID3D11DepthStencilState* Renderer::m_pDepthStateEnable = nullptr;
ID3D11DepthStencilState* Renderer::m_pDepthStateDisable = nullptr;
HWND Application::m_hWnd = nullptr;

namespace {
    std::array<ComPtr<ID3D11Buffer>, 6> constantBuffers;

#ifdef _DEBUG
    void CheckMissileBlastHistory() {
        ExplosionManager manager;
        assert(!manager.m_debugShowMissileBlast && manager.GetDisplayedDebugMissileBlast() == nullptr);
        MissileBlastDebugSample impact;
        impact.center = {1, 2, 3};
        impact.hits.push_back({{4, 5, 6}, 1, false});
        impact.outerHits = 1;
        for (unsigned shot = 1; shot <= 20; ++shot) {
            manager.RecordDebugMissileBlast(impact);
            assert(manager.GetDisplayedDebugMissileBlast()->id == shot);
        }
        manager.m_debugSelectedBlastId = 2;
        manager.RecordDebugMissileBlast(impact);
        assert(manager.GetDebugMissileBlasts().size() == 20);
        assert(manager.GetDebugMissileBlasts().front().id == 2);
        assert(manager.GetDisplayedDebugMissileBlast()->id == 2);
        const auto& hit = manager.GetDisplayedDebugMissileBlast()->hits.front();
        assert(hit.center == Vector3(4, 5, 6) && !hit.full && hit.damage == 1);
        manager.RecordDebugMissileBlast(impact);
        assert(manager.m_debugSelectedBlastId == 0 && manager.GetDisplayedDebugMissileBlast()->id == 22);
        manager.m_debugSelectedBlastId = 3;
        manager.ClearDebugMissileBlasts();
        assert(manager.m_debugSelectedBlastId == 0 && manager.GetDisplayedDebugMissileBlast() == nullptr);
        manager.RecordDebugMissileBlast(impact);
        assert(manager.GetDebugMissileBlasts().size() == 1 &&
               manager.GetDisplayedDebugMissileBlast()->id == 23);
        ExplosionManager nextStage;
        assert(nextStage.GetDebugMissileBlasts().empty());
        nextStage.RecordDebugMissileBlast(impact);
        assert(nextStage.GetDisplayedDebugMissileBlast()->id == 1);
    }
#endif

    ComPtr<ID3DBlob> Compile(const char* path, const char* model) {
        ComPtr<ID3DBlob> code, errors;
        const auto wide = std::filesystem::path(path).wstring();
        const HRESULT hr = D3DCompileFromFile(wide.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                              "main", model, 0, 0, &code, &errors);
        if (errors) {
            std::cerr << static_cast<const char*>(errors->GetBufferPointer());
        }
        assert(SUCCEEDED(hr));
        return code;
    }

    void SetMatrix(UINT slot, const Matrix& matrix) {
        const Matrix transposed = matrix.Transpose();
        auto context = Renderer::GetDeviceContext();
        context->UpdateSubresource(constantBuffers[slot].Get(), 0, nullptr, &transposed, 0, 0);
        context->VSSetConstantBuffers(slot, 1, constantBuffers[slot].GetAddressOf());
    }
} // namespace

bool Renderer::CreateConstantBuffer(unsigned size, ID3D11Buffer** buffer) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    return SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, buffer));
}

bool Renderer::CreateVertexBufferWrite(unsigned stride, unsigned count, void* data, ID3D11Buffer** buffer) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = stride * count;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA initial{data};
    return SUCCEEDED(m_pDevice->CreateBuffer(&desc, &initial, buffer));
}

bool Renderer::CreateIndexBuffer(unsigned count, void* data, ID3D11Buffer** buffer) {
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(unsigned) * count;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initial{data};
    return SUCCEEDED(m_pDevice->CreateBuffer(&desc, &initial, buffer));
}

HRESULT Renderer::CreateVertexShader(ID3D11VertexShader** shader, ID3D11InputLayout** layout,
                                     D3D11_INPUT_ELEMENT_DESC* elements, unsigned count, const char* path) {
    const auto code = Compile(path, "vs_5_0");
    HRESULT hr =
        m_pDevice->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, shader);
    assert(SUCCEEDED(hr));
    return m_pDevice->CreateInputLayout(elements, count, code->GetBufferPointer(), code->GetBufferSize(),
                                        layout);
}

HRESULT Renderer::CreatePixelShader(ID3D11PixelShader** shader, const char* path) {
    const auto code = Compile(path, "ps_5_0");
    return m_pDevice->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, shader);
}

void Renderer::SetWorldMatrix(Matrix* world) {
    SetMatrix(0, *world);
}

void Renderer::SetUV(float u, float v, float width, float height) {
    SetMatrix(5, Matrix::CreateScale(width, height, 1) * Matrix::CreateTranslation(u, v, 0));
}

void Renderer::SetDepthEnable(bool writesDepth) {
    // 本体と同じく false は深度テストを維持し、書き込みだけを止める。
    m_pDeviceContext->OMSetDepthStencilState(writesDepth ? m_pDepthStateEnable : m_pDepthStateDisable, 0);
}

void Camera::SetCamera(int) {
    SetMatrix(1, Matrix::Identity);
    SetMatrix(2, Matrix::Identity);
}

HRESULT Renderer::Init() {
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                   &m_pDevice, nullptr, &m_pDeviceContext);
    assert(SUCCEEDED(hr));
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = TRUE;
    depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    assert(SUCCEEDED(m_pDevice->CreateDepthStencilState(&depth, &m_pDepthStateEnable)));
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    assert(SUCCEEDED(m_pDevice->CreateDepthStencilState(&depth, &m_pDepthStateDisable)));
    for (int i = 0; i < MAX_BLENDSTATE; ++i) {
        D3D11_BLEND_DESC blend{};
        auto& target = blend.RenderTarget[0];
        target.BlendEnable = i != BS_NONE;
        target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        target.DestBlend = i == BS_ADDITIVE ? D3D11_BLEND_ONE : D3D11_BLEND_INV_SRC_ALPHA;
        target.BlendOp = D3D11_BLEND_OP_ADD;
        target.SrcBlendAlpha = D3D11_BLEND_ONE;
        target.DestBlendAlpha = D3D11_BLEND_ZERO;
        target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        assert(SUCCEEDED(m_pDevice->CreateBlendState(&blend, &m_pBlendState[i])));
    }
    for (UINT slot : {0u, 1u, 2u, 5u}) {
        assert(CreateConstantBuffer(sizeof(Matrix), &constantBuffers[slot]));
    }
    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID;
    raster.CullMode = D3D11_CULL_NONE;
    raster.DepthClipEnable = TRUE;
    ComPtr<ID3D11RasterizerState> rasterState;
    assert(SUCCEEDED(m_pDevice->CreateRasterizerState(&raster, &rasterState)));
    m_pDeviceContext->RSSetState(rasterState.Get());
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> samplerState;
    assert(SUCCEEDED(m_pDevice->CreateSamplerState(&sampler, &samplerState)));
    m_pDeviceContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());
    return hr;
}

void Renderer::Uninit() {
    m_pDeviceContext->ClearState();
    for (auto& buffer : constantBuffers) {
        buffer.Reset();
    }
    for (auto blend : m_pBlendState) {
        blend->Release();
    }
    m_pDepthStateEnable->Release();
    m_pDepthStateDisable->Release();
    m_pDeviceContext->Release();
    m_pDevice->Release();
}

int main() {
#ifdef _DEBUG
    CheckMissileBlastHistory();
#endif
    assert(SUCCEEDED(Renderer::Init()));
    {
        auto device = Renderer::GetDevice();
        auto context = Renderer::GetDeviceContext();
        constexpr UINT size = 64;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = desc.Height = size;
        desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> color, staging, depth;
        ComPtr<ID3D11RenderTargetView> target;
        ComPtr<ID3D11DepthStencilView> depthView;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &color)));
        assert(SUCCEEDED(device->CreateRenderTargetView(color.Get(), nullptr, &target)));
        desc.BindFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)));
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &depth)));
        assert(SUCCEEDED(device->CreateDepthStencilView(depth.Get(), nullptr, &depthView)));
        const D3D11_VIEWPORT viewport{0, 0, size, size, 0, 1};
        context->RSSetViewports(1, &viewport);

        // 背景・手前の物体用。爆発はこのテスト用シェーダーを使わない。
        const char* shader = R"(
cbuffer Geometry : register(b0) { float4 color; float depth; };
float4 VS(uint id : SV_VertexID) : SV_POSITION {
    float2 p[3] = {float2(-1,-1),float2(-1,3),float2(3,-1)};
    return float4(p[id], depth, 1);
}
float4 PS() : SV_Target { return color; }
)";
        ComPtr<ID3DBlob> code;
        ComPtr<ID3D11VertexShader> solidVS;
        ComPtr<ID3D11PixelShader> solidPS;
        ComPtr<ID3D11Buffer> geometry;
        assert(SUCCEEDED(D3DCompile(shader, strlen(shader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0,
                                    &code, nullptr)));
        assert(SUCCEEDED(
            device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &solidVS)));
        code.Reset();
        assert(SUCCEEDED(D3DCompile(shader, strlen(shader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0,
                                    &code, nullptr)));
        assert(SUCCEEDED(
            device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &solidPS)));
        assert(Renderer::CreateConstantBuffer(32, &geometry));
        const auto solid = [&](float z, const Color& tint) {
            const float data[]{tint.x, tint.y, tint.z, tint.w, z, 0, 0, 0};
            context->UpdateSubresource(geometry.Get(), 0, nullptr, data, 0, 0);
            context->VSSetConstantBuffers(0, 1, geometry.GetAddressOf());
            context->PSSetConstantBuffers(0, 1, geometry.GetAddressOf());
            context->VSSetShader(solidVS.Get(), nullptr, 0);
            context->PSSetShader(solidPS.Get(), nullptr, 0);
            context->IASetInputLayout(nullptr);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            Renderer::SetDepthEnable(true);
            Renderer::SetBlendState(BS_NONE);
            context->Draw(3, 0);
        };
        const auto begin = [&] {
            context->OMSetRenderTargets(1, target.GetAddressOf(), depthView.Get());
            const float black[]{0, 0, 0, 1};
            context->ClearRenderTargetView(target.Get(), black);
            context->ClearDepthStencilView(depthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
        };
        const auto pixels = [&] {
            context->CopyResource(staging.Get(), color.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            assert(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            std::vector<unsigned char> bytes;
            for (UINT y = 0; y < size; ++y) {
                auto row = static_cast<unsigned char*>(mapped.pData) + y * mapped.RowPitch;
                bytes.insert(bytes.end(), row, row + size * 4);
            }
            context->Unmap(staging.Get(), 0);
            return bytes;
        };

        Camera camera;
        billboard::LoadTextures("test_explosion", "assets/texture/PNG/Explosion/Explosion", ".png", 1);
        billboard explosion;
        explosion.SetTextureGroup("test_explosion");
        explosion.Init();
        explosion.SetSpriteSheet(4, 4, 16);
        explosion.SetAnim(1.0f / 60.0f, false);
        explosion.SetScale(2.0f);
        explosion.Play(Vector3(0, 0, 0.4f));
        unsigned visibleFrames = 0;
        for (int frame = 0; frame < 16; ++frame) {
            // 後から描かれる敵の青色が、透明な四角形で欠けないこと。
            begin();
            explosion.Draw(&camera);
            solid(0.8f, Color(0, 0, 1, 1));
            const auto lateBackground = pixels();
            for (size_t i = 0; i < lateBackground.size(); i += 4) {
                assert(lateBackground[i] == 0 && lateBackground[i + 1] == 0 && lateBackground[i + 2] == 255);
            }

            // 手前の壁は爆発を遮る。深度テストを無効にするだけの修正を検出する。
            begin();
            solid(0.2f, Color(1, 0, 0, 1));
            const auto foreground = pixels();
            explosion.Draw(&camera);
            assert(pixels() == foreground);

            // 本来の描画順で、奥の物体を残したまま爆発の光が加算される。
            begin();
            solid(0.8f, Color(0, 0, 0.2f, 1));
            const auto background = pixels();
            explosion.Draw(&camera);
            const auto composed = pixels();
            unsigned changedPixels = 0;
            unsigned transparentPixels = 0;
            for (size_t i = 0; i < composed.size(); i += 4) {
                assert(composed[i + 2] >= background[i + 2]);
                if (composed[i] != background[i] || composed[i + 1] != background[i + 1] ||
                    composed[i + 2] != background[i + 2]) {
                    ++changedPixels;
                } else {
                    ++transparentPixels;
                }
            }
            visibleFrames += changedPixels > 0;
            assert(transparentPixels > 0);

            // 呼び出し元のステート参照値・ブレンド係数・サンプルマスクも戻す。
            ComPtr<ID3D11DepthStencilState> previousDepth, restoredDepth;
            ComPtr<ID3D11BlendState> previousBlend, restoredBlend;
            UINT reference = 0, restoredReference = 0, mask = 0;
            float ignored[4], restoredFactors[4];
            context->OMGetDepthStencilState(&previousDepth, &reference);
            context->OMGetBlendState(&previousBlend, ignored, &mask);
            const float factors[]{0.1f, 0.2f, 0.3f, 0.4f};
            context->OMSetDepthStencilState(previousDepth.Get(), 37);
            context->OMSetBlendState(previousBlend.Get(), factors, 0xffffffefu);
            explosion.Draw(&camera);
            context->OMGetDepthStencilState(&restoredDepth, &restoredReference);
            context->OMGetBlendState(&restoredBlend, restoredFactors, &mask);
            assert(previousDepth.Get() == restoredDepth.Get() && restoredReference == 37);
            assert(previousBlend.Get() == restoredBlend.Get() && mask == 0xffffffefu);
            for (int i = 0; i < 4; ++i) {
                assert(factors[i] == restoredFactors[i]);
            }
            GameTime::BeginFrame(1.0 / 60.0, 1, false);
            assert(GameTime::NextStep());
            explosion.Update();
        }
        assert(visibleFrames > 0);
        billboard::ReleaseTextures("test_explosion");
    }
    Renderer::Uninit();
    std::cout << "16 explosion frames: background visibility, foreground occlusion, additive light and state "
                 "restoration passed.\n";
}
