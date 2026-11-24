/// @file StageSunShadowTests.cpp
/// @brief 太陽から見た深度と森の地面を WARP で描き、影の位置と環境光を読み戻す。

#include "../DX22_01_plane/StageSunShadows.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

ID3D11Device* Renderer::m_pDevice = nullptr;
ID3D11DeviceContext* Renderer::m_pDeviceContext = nullptr;

namespace {
    std::array<ComPtr<ID3D11Buffer>, 3> transforms;

    ComPtr<ID3D11Buffer> CreateBuffer(UINT size, UINT flags, const void* initial = nullptr) {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = size;
        desc.BindFlags = flags;
        D3D11_SUBRESOURCE_DATA data{initial};
        ComPtr<ID3D11Buffer> buffer;
        assert(SUCCEEDED(Renderer::GetDevice()->CreateBuffer(&desc, initial ? &data : nullptr, &buffer)));
        return buffer;
    }

    void SetTransform(UINT slot, const Matrix& transform) {
        auto context = Renderer::GetDeviceContext();
        const Matrix transposed = transform.Transpose();
        context->UpdateSubresource(transforms[slot].Get(), 0, nullptr, &transposed, 0, 0);
        context->VSSetConstantBuffers(slot, 1, transforms[slot].GetAddressOf());
    }

    ComPtr<ID3DBlob> Compile(const wchar_t* file, const char* profile) {
        ComPtr<ID3DBlob> code, errors;
        const HRESULT result = D3DCompileFromFile(file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                                                  profile, 0, 0, &code, &errors);
        if (errors) {
            std::cerr << static_cast<const char*>(errors->GetBufferPointer());
        }
        assert(SUCCEEDED(result));
        return code;
    }
} // namespace

HRESULT Renderer::Init() {
    return D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                             &m_pDevice, nullptr, &m_pDeviceContext);
}

void Renderer::SetViewMatrix(Matrix* matrix) {
    SetTransform(1, *matrix);
}

void Renderer::SetProjectionMatrix(Matrix* matrix) {
    SetTransform(2, *matrix);
}

void Renderer::SetDepthEnable(bool enable) {
    assert(enable);
    m_pDeviceContext->OMSetDepthStencilState(nullptr, 0); // D3D11 の既定は深度テスト・書き込みとも有効。
}

void Renderer::Uninit() {
    m_pDeviceContext->ClearState();
    transforms = {};
    m_pDeviceContext->Release();
    m_pDevice->Release();
}

int main() {
    assert(SUCCEEDED(Renderer::Init()));
    auto device = Renderer::GetDevice();
    auto context = Renderer::GetDeviceContext();
    StageSunShadows shadows;
    assert(shadows.Init());

    const auto vsCode = Compile(L"shader/groundSkyVS.hlsl", "vs_5_0");
    const auto psCode = Compile(L"shader/forestGroundPS.hlsl", "ps_5_0");
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    assert(SUCCEEDED(
        device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vs)));
    assert(SUCCEEDED(
        device->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &ps)));
    const D3D11_INPUT_ELEMENT_DESC elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VERTEX_3D, position),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VERTEX_3D, normal),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(VERTEX_3D, color),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VERTEX_3D, uv), D3D11_INPUT_PER_VERTEX_DATA,
         0}};
    ComPtr<ID3D11InputLayout> layout;
    assert(SUCCEEDED(device->CreateInputLayout(elements, std::size(elements), vsCode->GetBufferPointer(),
                                               vsCode->GetBufferSize(), &layout)));
    // 同じ板を縮尺と位置だけ変えて遮蔽物・地面に使う。地面は上から正射影で観察する。
    const VERTEX_3D vertices[] = {{{-1, 0, 1}, Vector3::Up, Color(1, 1, 1, 0), {}},
                                  {{1, 0, 1}, Vector3::Up, Color(1, 1, 1, 0), {}},
                                  {{-1, 0, -1}, Vector3::Up, Color(1, 1, 1, 0), {}},
                                  {{1, 0, -1}, Vector3::Up, Color(1, 1, 1, 0), {}}};
    auto quad = CreateBuffer(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER, vertices);
    UINT stride = sizeof(VERTEX_3D), offset = 0;
    context->IASetInputLayout(layout.Get());
    context->IASetVertexBuffers(0, 1, quad.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->VSSetShader(vs.Get(), nullptr, 0);
    context->PSSetShader(ps.Get(), nullptr, 0);
    for (auto& transform : transforms) {
        transform = CreateBuffer(sizeof(Matrix), D3D11_BIND_CONSTANT_BUFFER);
    }
    auto lightBuffer = CreateBuffer(sizeof(LIGHT), D3D11_BIND_CONSTANT_BUFFER);
    context->VSSetConstantBuffers(3, 1, lightBuffer.GetAddressOf());
    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    auto materialBuffer = CreateBuffer(sizeof(material), D3D11_BIND_CONSTANT_BUFFER, &material);
    context->PSSetConstantBuffers(4, 1, materialBuffer.GetAddressOf());

    constexpr UINT width = 64;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = desc.Height = width;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> color, staging;
    ComPtr<ID3D11RenderTargetView> target;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &color)));
    assert(SUCCEEDED(device->CreateRenderTargetView(color.Get(), nullptr, &target)));
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)));
    context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
    const D3D11_VIEWPORT viewport{0, 0, float(width), float(width), 0, 1};
    context->RSSetViewports(1, &viewport);

    const Vector3 center(120, 5, -180);
    const auto drawQuad = [&](const Vector3& position, float halfWidth) {
        SetTransform(0, Matrix::CreateScale(halfWidth) * Matrix::CreateTranslation(position));
        context->Draw(4, 0);
    };
    const auto sampleGround = [&](const Vector3& position) {
        SetTransform(1, DirectX::XMMatrixLookAtLH(position + Vector3(0, 25, 0), position, Vector3::UnitZ));
        SetTransform(2, DirectX::XMMatrixOrthographicLH(32, 32, 1, 50));
        const float clear[4]{};
        context->ClearRenderTargetView(target.Get(), clear);
        shadows.BindGround();
        drawQuad(position, 16);
        shadows.UnbindGround();
        context->CopyResource(staging.Get(), color.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        assert(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
        const auto row = reinterpret_cast<const float*>(static_cast<const char*>(mapped.pData) +
                                                        width / 2 * mapped.RowPitch);
        const std::array<float, 2> samples{row[width / 4], row[3 * width / 4]};
        context->Unmap(staging.Get(), 0);
        return samples;
    };

    const float ambient = 0.2f;
    const float lit = ambient + 0.8f / std::sqrt(2.0f);
    const auto matches = [](float actual, float expected) { return std::abs(actual - expected) < 0.01f; };
    for (const float horizontal : {1.0f, -1.0f}) {
        Vector3 direction(horizontal, -1, 0);
        direction.Normalize();
        LIGHT light{};
        light.Direction = Vector4(direction.x, direction.y, direction.z, 0);
        light.Diffuse = Color(0.8f, 0.8f, 0.8f, 1);
        light.Ambient = Color(ambient, ambient, ambient, 1);
        context->UpdateSubresource(lightBuffer.Get(), 0, nullptr, &light, 0, 0);
        shadows.Capture(center, direction, [&] { drawQuad(center + Vector3(0, 8, 0), 4); });
        const auto ground = sampleGround(center);
        // 高さ8の板は45度の日差しで真下から8ずれる。影の中には環境光が残る。
        assert(matches(ground[0], horizontal < 0 ? ambient : lit));
        assert(matches(ground[1], horizontal > 0 ? ambient : lit));
        // 同じ光線上でもマップの遠端を越えた場所に影を引き延ばさない。
        for (auto outside : sampleGround(center + direction * 20000)) {
            assert(matches(outside, lit));
        }
    }
    shadows.Capture(center, Vector3::Zero, [] { assert(false); });
    for (auto unshadowed : sampleGround(center)) {
        assert(matches(unshadowed, lit));
    }
    Renderer::Uninit();
    std::cout << "Forest sunlight, moving shadows, ambient light and map bounds passed.\n";
}
