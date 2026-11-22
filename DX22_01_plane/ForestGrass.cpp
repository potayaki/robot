/// @brief 地表に根を置いた草株を作成し、可視区画をまとめて描画する。
#include "ForestGrass.h"
#include "AimCursor.h"
#include "Application.h"
#include "Camera.h"
#include "ForestLayout.h"
#include "ForestMeadow.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace {
    struct Blade {
        /// ワールド座標の根元と、色・揺らぎに使う0～1の乱数。
        Vector4 root;
        /// 葉の高さ、半幅、方位（ラジアン）、静止時の葉先の倒れ幅。
        Vector4 traits;
    };

    static_assert(sizeof(Blade) == 32);

    constexpr int TileSize = 160;
    constexpr int CellsPerSide = 21;
    constexpr int MeadowHalfWidth = 1280;
    constexpr int MeadowStartZ = -1440;
    constexpr float CellSize = static_cast<float>(TileSize) / CellsPerSide;
    constexpr float FadeEnd = 620.0f; // forestGrassVSが葉を縮め切る視距離。
    constexpr UINT VerticesPerBlade = 9;
} // namespace

bool ForestGrass::Init() {
    *this = ForestGrass{};
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"GRASS_ROOT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        {"GRASS_TRAITS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    };
    if (FAILED(Renderer::CreateVertexShader(m_vertexShader.GetAddressOf(), m_layout.GetAddressOf(), layout,
                                            ARRAYSIZE(layout), "shader/forestGrassVS.hlsl")) ||
        FAILED(Renderer::CreatePixelShader(m_pixelShader.GetAddressOf(), "shader/forestGrassPS.hlsl"))) {
        return false;
    }
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    auto device = Renderer::GetDevice();
    if (FAILED(device->CreateRasterizerState(&rasterizer, m_rasterizer.GetAddressOf()))) {
        return false;
    }

    std::mt19937 random(62041);
    const auto unit = [&] { return static_cast<float>(random() >> 8) / 16777216.0f; };
    std::vector<Blade> blades;
    constexpr int tileColumns = 2 * MeadowHalfWidth / TileSize;
    constexpr int tileRows =
        (static_cast<int>(ForestLayout::CampMaxZ) - MeadowStartZ + TileSize - 1) / TileSize;
    // 道や野営地の空きも含め、1セルあたり約5枚を先に確保する。
    blades.reserve(tileColumns * tileRows * CellsPerSide * CellsPerSide * 5);
    // 1セルの中で株の中心をずらす。葉は株ごとに近い背丈を持ち、根元から別々の方向へ倒れる。
    for (int z = MeadowStartZ; z < ForestLayout::CampMaxZ; z += TileSize) {
        for (int x = -MeadowHalfWidth; x < MeadowHalfWidth; x += TileSize) {
            const auto first = static_cast<UINT>(blades.size());
            float low = (std::numeric_limits<float>::max)();
            float high = std::numeric_limits<float>::lowest();
            for (int row = 0; row < CellsPerSide; ++row) {
                for (int col = 0; col < CellsPerSide; ++col) {
                    const float cx = x + (col + unit()) * CellSize;
                    const float cz = z + (row + unit()) * CellSize;
                    const float cover = ForestMeadow::Cover(cx, cz);
                    if (unit() >= cover) {
                        continue;
                    }
                    const float height = 4.4f + 4.1f * unit();
                    const float facing = DirectX::XM_2PI * unit();
                    const int count = 4 + static_cast<int>(random() % 4);
                    for (int i = 0; i < count; ++i) {
                        const float yaw = facing + i * 2.40f + unit() * 0.6f;
                        const float spread = 0.5f + unit() * 3.5f;
                        const float bx = cx + std::cos(yaw) * spread;
                        const float bz = cz + std::sin(yaw) * spread;
                        // 密生域は境界まで株の広がり以上の余裕がある。境界付近だけ各葉を調べる。
                        if (cover < 1.0f && ForestMeadow::Cover(bx, bz) <= 0.0f) {
                            continue;
                        }
                        const float by = ForestLayout::GroundHeight(bx, bz) - 0.22f;
                        const float length = height * (0.78f + 0.45f * unit());
                        const float halfWidth = length * (0.034f + 0.028f * unit());
                        blades.push_back({{bx, by, bz, unit()},
                                          {length, halfWidth, yaw, length * (0.09f + 0.26f * unit())}});
                        low = (std::min)(low, by);
                        high = (std::max)(high, by + length);
                    }
                }
            }
            const auto count = static_cast<UINT>(blades.size()) - first;
            if (count != 0) {
                // 株の広がりと風で葉先が区画の辺を越える分も、境界に含める。
                constexpr float half = TileSize * 0.5f;
                const float halfHeight = (high - low) * 0.5f;
                const float radius = std::sqrt(2.0f * half * half + halfHeight * halfHeight) + 16.0f;
                m_tiles.push_back({{x + half, low + halfHeight, z + half}, radius, first, count});
            }
        }
    }
    if (blades.empty()) {
        return false;
    }
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(sizeof(Blade) * blades.size());
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = blades.data();
    return SUCCEEDED(device->CreateBuffer(&desc, &initial, m_instances.GetAddressOf()));
}

void ForestGrass::Draw(Camera* camera) const {
    if (!camera || !m_instances) {
        return;
    }
    // 呼び出し側が設定した3Dカメラと風の定数バッファを、地表と共有する。
    const Matrix view = camera->GetViewMatrix();
    const float tanY = std::tan(AimCursor::FieldOfView * 0.5f);
    const float tanX = tanY * Application::GetWidth() / (std::max)(1u, Application::GetHeight());
    auto context = Renderer::GetDeviceContext();
    ComPtr<ID3D11VertexShader> oldVS;
    ComPtr<ID3D11PixelShader> oldPS;
    ComPtr<ID3D11InputLayout> oldLayout;
    ComPtr<ID3D11Buffer> oldBuffer;
    ComPtr<ID3D11RasterizerState> oldRasterizer;
    D3D11_PRIMITIVE_TOPOLOGY oldTopology;
    UINT oldStride, oldOffset;
    context->VSGetShader(oldVS.GetAddressOf(), nullptr, nullptr);
    context->PSGetShader(oldPS.GetAddressOf(), nullptr, nullptr);
    context->IAGetInputLayout(oldLayout.GetAddressOf());
    context->IAGetVertexBuffers(0, 1, oldBuffer.GetAddressOf(), &oldStride, &oldOffset);
    context->IAGetPrimitiveTopology(&oldTopology);
    context->RSGetState(oldRasterizer.GetAddressOf());

    constexpr UINT stride = sizeof(Blade);
    constexpr UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_instances.GetAddressOf(), &stride, &offset);
    context->IASetInputLayout(m_layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->RSSetState(m_rasterizer.Get());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    for (const auto& tile : m_tiles) {
        const Vector3 center = Vector3::Transform(tile.center, view);
        const float farZ = center.z + tile.radius;
        const float farDistance = FadeEnd + tile.radius;
        if (farZ > 0 && center.LengthSquared() < farDistance * farDistance &&
            std::abs(center.x) <= farZ * tanX + tile.radius &&
            std::abs(center.y) <= farZ * tanY + tile.radius) {
            context->DrawInstanced(VerticesPerBlade, tile.count, 0, tile.first);
        }
    }
    context->VSSetShader(oldVS.Get(), nullptr, 0);
    context->PSSetShader(oldPS.Get(), nullptr, 0);
    context->IASetInputLayout(oldLayout.Get());
    context->IASetVertexBuffers(0, 1, oldBuffer.GetAddressOf(), &oldStride, &oldOffset);
    context->IASetPrimitiveTopology(oldTopology);
    context->RSSetState(oldRasterizer.Get());
}
