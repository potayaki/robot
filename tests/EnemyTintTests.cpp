/// @file EnemyTintTests.cpp
/// @brief 実際の個体色シェーダーをWARPで描き、模様・透明度・描画状態の復元を検証する。

#include "../DX22_01_plane/EnemyTint.h"
#include "../DX22_01_plane/Renderer.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>

using DirectX::SimpleMath::Color;
using Microsoft::WRL::ComPtr;

ID3D11Device* Renderer::m_pDevice = nullptr;
ID3D11DeviceContext* Renderer::m_pDeviceContext = nullptr;

HRESULT Renderer::Init() {
    return D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                             &m_pDevice, nullptr, &m_pDeviceContext);
}

void Renderer::Uninit() {
    m_pDeviceContext->ClearState();
    m_pDeviceContext->Release();
    m_pDevice->Release();
}

HRESULT Renderer::CreatePixelShader(ID3D11PixelShader** shader, const char* path) {
    ComPtr<ID3DBlob> code, errors;
    const auto wide = std::filesystem::path(path).wstring();
    const HRESULT result = D3DCompileFromFile(wide.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                              "main", "ps_5_0", 0, 0, &code, &errors);
    if (errors) {
        std::cerr << static_cast<const char*>(errors->GetBufferPointer());
    }
    assert(SUCCEEDED(result));
    return m_pDevice->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, shader);
}

int main() {
    assert(SUCCEEDED(Renderer::Init()));
    {
        auto device = Renderer::GetDevice();
        auto context = Renderer::GetDeviceContext();
        const auto tintResources = EnemyTint::Acquire();
        assert(tintResources && EnemyTint::Acquire() == tintResources);

        // 赤い表面・暗い模様・透明な黒・明るい模様。実シェーダーに渡して比較する。
        const std::array<Color, 4> texels{{Color(1, 0.04f, 0.01f, 0.35f), Color(0.1f, 0.004f, 0.001f, 0.65f),
                                           Color(0, 0, 0, 0), Color(0.8f, 0.8f, 0.8f, 1)}};
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = 4;
        desc.Height = desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{texels.data(), sizeof(texels), 0};
        ComPtr<ID3D11Texture2D> texture, target, staging;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11RenderTargetView> rtv;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, &data, &texture)));
        assert(SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, &srv)));
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)));
        assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
        desc.BindFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)));

        const char* vertexSource = R"(
struct Pixel { float4 pos : SV_POSITION; float4 col : COLOR0; float2 tex : TEXCOORD0; };
Pixel main(uint id : SV_VertexID) {
    float2 positions[3] = {float2(-1,-1), float2(-1,3), float2(3,-1)};
    Pixel result;
    result.pos = float4(positions[id], 0, 1);
    result.col = float4(1,1,1,1);
    result.tex = result.pos.xy * float2(0.5,-0.5) + 0.5;
    return result;
})";
        ComPtr<ID3DBlob> vertexCode;
        assert(SUCCEEDED(D3DCompile(vertexSource, strlen(vertexSource), nullptr, nullptr, nullptr, "main",
                                    "vs_5_0", 0, 0, &vertexCode, nullptr)));
        ComPtr<ID3D11VertexShader> vertexShader;
        assert(SUCCEEDED(device->CreateVertexShader(vertexCode->GetBufferPointer(),
                                                    vertexCode->GetBufferSize(), nullptr, &vertexShader)));
        ComPtr<ID3D11PixelShader> normalShader, unlitShader, warningShader;
        assert(SUCCEEDED(Renderer::CreatePixelShader(&normalShader, "shader/litTexturePS.hlsl")));
        assert(SUCCEEDED(Renderer::CreatePixelShader(&unlitShader, "shader/unlitTexturePS.hlsl")));
        assert(SUCCEEDED(Renderer::CreatePixelShader(&warningShader, "shader/mushroomWarningPS.hlsl")));
        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(MATERIAL);
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ComPtr<ID3D11Buffer> materialBuffer, previousTintBuffer;
        assert(SUCCEEDED(device->CreateBuffer(&bufferDesc, nullptr, &materialBuffer)));
        bufferDesc.ByteWidth = 32;
        assert(SUCCEEDED(device->CreateBuffer(&bufferDesc, nullptr, &previousTintBuffer)));
        MATERIAL material{};
        material.TextureEnable = TRUE;
        material.Diffuse = Color(0.1f, 0.9f, 0.2f, 0.75f);
        context->UpdateSubresource(materialBuffer.Get(), 0, nullptr, &material, 0, 0);
        context->PSSetConstantBuffers(4, 1, materialBuffer.GetAddressOf());

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> sampler;
        assert(SUCCEEDED(device->CreateSamplerState(&samplerDesc, &sampler)));
        context->PSSetSamplers(0, 1, sampler.GetAddressOf());
        context->PSSetShaderResources(0, 1, srv.GetAddressOf());
        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        ComPtr<ID3D11RasterizerState> raster;
        assert(SUCCEEDED(device->CreateRasterizerState(&rasterDesc, &raster)));
        context->RSSetState(raster.Get());
        const D3D11_VIEWPORT viewport{0, 0, 4, 1, 0, 1};
        context->RSSetViewports(1, &viewport);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
        const auto draw = [&](ID3D11PixelShader* shader, const Color& tint, bool textureOnly = false) {
            context->PSSetShader(shader, nullptr, 0);
            context->PSSetConstantBuffers(7, 1, previousTintBuffer.GetAddressOf());
            {
                EnemyTintScope scope(tintResources, tint, textureOnly);
                context->Draw(3, 0);
            }
            ComPtr<ID3D11PixelShader> restoredShader;
            ComPtr<ID3D11Buffer> restoredBuffer;
            context->PSGetShader(&restoredShader, nullptr, nullptr);
            context->PSGetConstantBuffers(7, 1, &restoredBuffer);
            assert(restoredShader.Get() == shader && restoredBuffer == previousTintBuffer);
            context->CopyResource(staging.Get(), target.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            assert(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
            std::array<Color, 4> pixels;
            memcpy(pixels.data(), mapped.pData, sizeof(pixels));
            context->Unmap(staging.Get(), 0);
            return pixels;
        };
        const Color normal(1, 1, 1, 0), blue(0.15f, 0.55f, 1, 0.9f), gold(1, 0.75f, 0.1f, 0.95f);
        const auto original = draw(normalShader.Get(), normal);
        const auto variant = draw(normalShader.Get(), blue);
        const auto rare = draw(normalShader.Get(), gold);
        for (size_t i = 0; i < texels.size(); ++i) {
            assert(original[i] == texels[i]);
            assert(variant[i].w == texels[i].w && rare[i].w == texels[i].w);
        }
        assert(variant[0].z > variant[0].x && variant[1].z > variant[1].x);
        assert(variant[0].z > variant[1].z && rare[0].x > rare[1].x);
        assert(rare[0].x > rare[0].z && variant[0] != rare[0]);
        assert(variant[2] == texels[2]); // 黒い目や透明部分の明暗を塗りつぶさない。
        assert(draw(normalShader.Get(), normal) == original); // 次の通常個体へ着色を残さない。

        material.TextureEnable = FALSE;
        context->UpdateSubresource(materialBuffer.Get(), 0, nullptr, &material, 0, 0);
        const auto materialTint = draw(normalShader.Get(), blue);
        assert(materialTint[0].w == material.Diffuse.w && materialTint[0] == materialTint[3]);
        assert(draw(unlitShader.Get(), blue, true) == variant); // Ghostは前の材質設定を参照しない。
        const auto warning = draw(warningShader.Get(), normal);
        assert(warning[0].x > warning[0].y && warning[0].x > warning[0].z);
        assert(draw(normalShader.Get(), Color(1, 1, 1, 0))[0] == material.Diffuse);
    }
    Renderer::Uninit();
    std::cout << "Enemy tint hue, surface detail, alpha, material paths and draw-state restoration passed.\n";
}
