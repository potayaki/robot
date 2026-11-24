/// @file LowHealthGlitchTests.cpp
/// @brief 動作と境界条件を検証する LowHealthGlitchTests の自動テスト。

// Standalone WARP render test: supplies the small Renderer API used by the effect.
#include "../DX22_01_plane/LowHealthGlitch.h"
#include "../DX22_01_plane/LowHealthGlitchStrength.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdlib>
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

int TotalChroma(const std::vector<unsigned char>& pixels) {
    int sum = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        sum += std::abs(pixels[i] - pixels[i + 1]) + std::abs(pixels[i + 1] - pixels[i + 2]) +
               std::abs(pixels[i + 2] - pixels[i]);
    }
    return sum;
}

void CheckQuietNoise(const std::vector<unsigned char>& mild, const std::vector<unsigned char>& severe,
                     UINT width, int background) {
    const size_t height = severe.size() / (4 * width);
    int mildNoise = 0, severeNoise = 0, grain = 0;
    int previousLeft = 0, previousRight = 0;
    size_t scanlines = 0;
    for (size_t y = 0; y < height; ++y) {
        int left = 0, right = 0;
        for (UINT x = 0; x < width; ++x) {
            const size_t i = (y * width + x) * 4;
            const int intensity = severe[i];
            mildNoise += std::abs(mild[i] - background);
            severeNoise += std::abs(intensity - background);
            assert(severe[i + 3] == 255);
            if (x > 0) {
                grain += std::abs(intensity - severe[i - 4]);
            }
            if (x < width / 2) {
                left += intensity;
            } else {
                right += intensity;
            }
        }
        // 行の左右で一緒に明滅する横線を、ランダムな粒の濃淡と区別する。
        const int halfWidth = static_cast<int>(width / 2);
        if (y > 0 && ((left - previousLeft > halfWidth && right - previousRight > halfWidth) ||
                      (previousLeft - left > halfWidth && previousRight - right > halfWidth))) {
            ++scanlines;
        }
        previousLeft = left;
        previousRight = right;
    }
    assert(TotalChroma(mild) == 0 && mildNoise > 0 && severeNoise > mildNoise);
    assert(severeNoise < static_cast<int>(width * height) * 32);
    assert(grain > static_cast<int>(width * height / 2) && scanlines >= height / 16);
}

unsigned CheckBackgroundStreaks(const std::vector<unsigned char>& pixels, UINT width, bool& sawTaper) {
    unsigned bins = 0;
    size_t colored = 0, verticalRuns = 0;
    std::vector<bool> coloredAbove(width);
    float previous = 0, beforePrevious = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        const int r = pixels[i], g = pixels[i + 1], b = pixels[i + 2];
        const int maximum = (std::max)({r, g, b});
        const float chroma = static_cast<float>(maximum - (std::min)({r, g, b}));
        const size_t x = i / 4 % width;
        const bool visible = chroma > 12;
        verticalRuns += visible && !coloredAbove[x];
        coloredAbove[x] = visible;
        // 横に連続した3画素で濃さが段階的に変わる、筋の先端を探す。
        if (x >= 2 && visible && previous > 12 && beforePrevious > 12) {
            sawTaper |= (beforePrevious < previous && previous < chroma) ||
                        (beforePrevious > previous && previous > chroma);
        }
        beforePrevious = previous;
        previous = chroma;
        assert(pixels[i + 3] == 255);
        if (!visible) {
            continue;
        }
        ++colored;
        // HSVの色相環を30度ずつ区切り、着色の濃淡を別の色相として数えない。
        float hue;
        if (maximum == r) {
            hue = (g - b) / chroma;
        } else if (maximum == g) {
            hue = 2 + (b - r) / chroma;
        } else {
            hue = 4 + (r - g) / chroma;
        }
        bins |= 1u << (static_cast<unsigned>((hue + 6) * 2) % 12);
    }
    // 108p・180pの無地面で発色は疎ら、縦の平均厚みは3ピクセル以内。
    assert(colored < (pixels.size() / 4) / 8);
    assert(colored <= verticalRuns * 3);
    return bins;
}

int main() {
    // 自動実行時も失敗した式と行番号をstderrに残し、ダイアログ待ちを避ける。
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    assert(LowHealthGlitchStrength(100, 100) == 0);
    assert(LowHealthGlitchStrength(50, 100) == 0);
    assert(std::abs(LowHealthGlitchStrength(49, 100) - 2.0f / 49) < 0.00001f);
    assert(std::abs(LowHealthGlitchStrength(25, 100) - 50.0f / 49) < 0.00001f);
    const float maximumStrength = LowHealthGlitchStrength(1, 100);
    assert(maximumStrength == 2);
    assert(LowHealthGlitchStrength(0, 100) == 2);
    assert(LowHealthGlitchStrength(-10, 100) == 2);
    assert(LowHealthGlitchStrength(10, 0) == 0);
    assert(std::abs(LowHealthGlitchStrength(50, 200) - 100.0f / 99) < 0.00001f);
    assert(std::abs(LowHealthGlitchStrength(50, 101) - 2.0f / 99) < 0.00001f);
    assert(LowHealthGlitchStrength(1, 200) == 2);
    assert(LowHealthGlitchStrength(1, 1) == 0);
    assert(LowHealthGlitchStrength(1, 2) == 0);
    assert(LowHealthGlitchStrength(0, 2) == 2);
    assert(SUCCEEDED(Renderer::Init()));
    {
        LowHealthGlitch glitch;
        auto device = Renderer::GetDevice();
        auto context = Renderer::GetDeviceContext();
        // Reuse the effect across different render sizes and MSAA modes.
        for (UINT samples : {1u, 4u}) {
            for (UINT width : {192u, 320u}) {
                const UINT height = width * 9 / 16;
                D3D11_TEXTURE2D_DESC desc{};
                desc.Width = width;
                desc.Height = height;
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
                D3D11_VIEWPORT viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
                context->RSSetViewports(1, &viewport);
                const auto render = [&](float strength, float time, const unsigned* input = nullptr) {
                    if (input) {
                        assert(samples == 1);
                        context->UpdateSubresource(target.Get(), 0, nullptr, input, width * 4, 0);
                    } else {
                        const float clear[] = {0.25f, 0.25f, 0.25f, 1};
                        context->ClearRenderTargetView(rtv.Get(), clear);
                    }
                    context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
                    glitch.Draw(strength, time);
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
                    for (UINT y = 0; y < height; ++y) {
                        const auto row =
                            static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch;
                        pixels.insert(pixels.end(), row, row + width * 4);
                    }
                    context->Unmap(staging.Get(), 0);
                    return pixels;
                };
                const auto clean = render(0, 0);
                // バーストの開始時刻や乱数に依存せず、無彩色の背景で強度と休止を比較する。
                int mildPeak = 0, severePeak = 0, maximumPeak = 0;
                float severeTime = 0;
                float firstBurstTail = 0;
                bool sawChangingNoise = false, sawDifferentPattern = false;
                bool sawTaper = false;
                unsigned hueBins = 0;
                std::vector<unsigned char> quiet, severe;
                for (int frame = 0; frame < 60; ++frame) {
                    const float time = frame / 30.0f;
                    const auto mild = render(0.25f, time);
                    mildPeak = (std::max)(mildPeak, TotalChroma(mild));
                    auto burst = render(1, time);
                    const auto maximum = render(maximumStrength, time);
                    const int maximumChroma = TotalChroma(maximum);
                    maximumPeak = (std::max)(maximumPeak, maximumChroma);
                    const int chroma = TotalChroma(burst);
                    if (chroma == 0) {
                        assert(burst != clean);
                        assert(maximumChroma == 0);
                        if (quiet.empty()) {
                            CheckQuietNoise(mild, burst, width, clean[0]);
                            CheckQuietNoise(burst, maximum, width, clean[0]);
                            assert(render(0, time) == clean);
                            quiet = burst;
                        } else {
                            sawChangingNoise |= quiet != burst;
                        }
                        continue;
                    }
                    if (quiet.empty()) {
                        firstBurstTail = time;
                    }
                    sawDifferentPattern |= !severe.empty() && severe != burst;
                    hueBins |= CheckBackgroundStreaks(burst, width, sawTaper);
                    if (chroma > severePeak) {
                        severePeak = chroma;
                        severeTime = time;
                        severe.swap(burst);
                    }
                }
                assert(sawChangingNoise && sawDifferentPattern);
                assert(mildPeak > 0 && severePeak > mildPeak && maximumPeak > severePeak);
                assert(sawTaper && std::popcount(hueBins) >= 8);
                assert(render(0, severeTime) == clean); // Recovery clears even an active burst.

                if (samples == 1) {
                    // 一本の白黒境界から画面幅の約3%離れた位置で、同じ側へ移った連続行を数える。
                    std::vector<unsigned> edge(width * height, 0xff000000);
                    for (UINT y = 0; y < height; ++y) {
                        std::fill_n(edge.begin() + y * width + width / 2, width / 2, 0xffffffff);
                    }
                    const auto shiftedRows = [&](float time) {
                        const auto pixels = render(1, time, edge.data());
                        UINT leftRun = 0, rightRun = 0, longest = 0;
                        for (UINT y = 0; y < height; ++y) {
                            const size_t center = y * width + width / 2;
                            const auto* left = &pixels[(center - width / 32) * 4];
                            const auto* right = &pixels[(center + width / 32) * 4];
                            leftRun = (std::min)({left[0], left[1], left[2]}) > 128 ? leftRun + 1 : 0;
                            rightRun = (std::max)({right[0], right[1], right[2]}) < 128 ? rightRun + 1 : 0;
                            longest = (std::max)({longest, leftRun, rightRun});
                        }
                        return longest;
                    };
                    const UINT initialRows = shiftedRows(0);
                    const UINT finalRows = shiftedRows(firstBurstTail);
                    assert(initialRows >= finalRows + height / 30);

                    // 色ずれで各チャンネルの境界は分かれるため、画面全体で明暗が移った面積を比べる。
                    const auto displacedArea = [&](float strength) {
                        const auto pixels = render(strength, 0, edge.data());
                        size_t area = 0;
                        for (size_t i = 0; i < edge.size(); ++i) {
                            const int brightness = pixels[4 * i] + pixels[4 * i + 1] + pixels[4 * i + 2];
                            const bool bright = brightness > 3 * 128;
                            area += bright != (edge[i] == 0xffffffff);
                        }
                        return area;
                    };
                    assert(displacedArea(maximumStrength) > displacedArea(1));

                    // 比較用の灰色背景に白い縦縞を足し、同じノイズのまま入力画像だけを変える。
                    std::vector<unsigned> stripes(width * height, 0xff404040);
                    for (UINT y = height / 4; y < height * 3 / 4; ++y) {
                        for (UINT x = 0; x < width; ++x) {
                            if ((x / 16) % 2 == 1) {
                                stripes[y * width + x] = 0xffffffff;
                            }
                        }
                    }
                    const auto striped = render(1, severeTime, stripes.data());
                    // 無彩色の縞から新しい色差が生じれば、輪郭に色の残像が出ている。
                    assert(TotalChroma(striped) > severePeak);
                    bool hasHorizontalTear = false;
                    for (UINT y = 0; y < height; ++y) {
                        UINT run = 0;
                        for (UINT x = 0; x < width; ++x) {
                            const UINT i = y * width + x;
                            // 全RGBが明るくなった灰色の画素には、白い縞自体が移動している。
                            const bool moved = stripes[i] == 0xff404040 &&
                                               striped[i * 4] > severe[i * 4] + 24 &&
                                               striped[i * 4 + 1] > severe[i * 4 + 1] + 24 &&
                                               striped[i * 4 + 2] > severe[i * 4 + 2] + 24;
                            if (y < height / 4 || y >= height * 3 / 4) {
                                assert(!moved);
                            }
                            run = moved ? run + 1 : 0;
                            hasHorizontalTear |= run >= 2;
                        }
                    }
                    assert(hasHorizontalTear);
                }
            }
        }
    }
    Renderer::Uninit();
    std::cout << "HP thresholds, monochrome grain, scanlines, multicolor bursts, horizontal tears, recovery, "
                 "resize and MSAA passed.\n";
}
