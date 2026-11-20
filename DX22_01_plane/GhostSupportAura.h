/// @file GhostSupportAura.h
/// @brief Ghostに支援されている個体の周囲へ、半透明の光の帯と上昇光を配置する。

#pragma once
#include "Renderer.h"
#include <cmath>
#include <span>

namespace GhostSupportAura {
    /// 敵の胴体を囲む帯の本数。
    inline constexpr size_t BandCount = 2;
    /// 光の帯を滑らかな円周へ分割する数。
    inline constexpr size_t SegmentCount = 24;
    /// 敵の周囲を上昇する光の数。
    inline constexpr size_t LightCount = 6;
    /// 透明な両端と明るい中央を持つ帯・光に必要な頂点数。
    inline constexpr size_t VertexCount = BandCount * SegmentCount * 12 + LightCount * 12;
    /// 波打つ帯と上昇光が一周するゲーム内の秒数。
    inline constexpr double CycleSeconds = 4.0;

    inline bool IsFinite(const DirectX::SimpleMath::Vector3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    // ワールド座標の判定球に合わせる。カメラの右・上方向は上昇光の向きにだけ使う。
    // 固定容量へ書き込み、描画中に頂点配列を確保しない。無効な入力なら何も書かない。
    inline size_t Build(std::span<VERTEX_3D> destination, DirectX::SimpleMath::Vector3 center, float radius,
                        double seconds, DirectX::SimpleMath::Vector3 cameraRight,
                        DirectX::SimpleMath::Vector3 cameraUp) {
        using namespace DirectX::SimpleMath;
        if (destination.size() < VertexCount || !IsFinite(center) || !std::isfinite(radius) || radius <= 0 ||
            !std::isfinite(seconds) || !IsFinite(cameraRight) || !IsFinite(cameraUp) ||
            cameraRight.LengthSquared() < 0.000001f || cameraUp.LengthSquared() < 0.000001f) {
            return 0;
        }
        cameraRight.Normalize();
        cameraUp.Normalize();
        const double cycles = seconds / CycleSeconds;
        const float cycle = static_cast<float>(cycles - std::floor(cycles));
        const float phase = cycle * DirectX::XM_2PI;
        const float pulse = 0.88f + 0.12f * std::sin(phase);
        size_t count = 0;
        const auto put = [&](const Vector3& position, const Color& color) {
            auto& vertex = destination[count++];
            vertex.position = position;
            vertex.normal = Vector3::Zero;
            vertex.color = color;
            vertex.uv = Vector2(0.5f, 0.5f);
        };

        for (size_t band = 0; band < BandCount; ++band) {
            const float direction = band == 0 ? 1.0f : -1.0f;
            const float baseHeight = radius * (band == 0 ? -0.35f : 0.35f);
            const float ringRadius =
                radius * (1.14f + 0.04f * std::sin(phase + static_cast<float>(band) * DirectX::XM_PI));
            const float halfWidth = radius * 0.12f;
            const auto point = [&](float angle) {
                return center + Vector3(std::cos(angle) * ringRadius,
                                        baseHeight + radius * 0.18f * std::sin(angle * 2 + direction * phase),
                                        std::sin(angle) * ringRadius);
            };
            const auto tint = [&](float angle, float feather) {
                const float arc = 0.65f + 0.35f * std::sin(angle - direction * phase);
                return Color(0.68f, 0.25f, 1.0f, feather * pulse * arc * 0.26f);
            };
            for (size_t segment = 0; segment < SegmentCount; ++segment) {
                const float angle0 =
                    DirectX::XM_2PI * static_cast<float>(segment) / static_cast<float>(SegmentCount);
                const float angle1 =
                    DirectX::XM_2PI * static_cast<float>(segment + 1) / static_cast<float>(SegmentCount);
                const Vector3 middle0 = point(angle0);
                const Vector3 middle1 = point(angle1);
                for (int edge : {-1, 1}) {
                    const Vector3 outside0 = middle0 + Vector3(0, halfWidth * static_cast<float>(edge), 0);
                    const Vector3 outside1 = middle1 + Vector3(0, halfWidth * static_cast<float>(edge), 0);
                    put(outside0, tint(angle0, 0));
                    put(middle0, tint(angle0, 1));
                    put(middle1, tint(angle1, 1));
                    put(outside0, tint(angle0, 0));
                    put(middle1, tint(angle1, 1));
                    put(outside1, tint(angle1, 0));
                }
            }
        }

        for (size_t light = 0; light < LightCount; ++light) {
            const float offset = static_cast<float>(light) / static_cast<float>(LightCount);
            const float rise = cycle + offset - std::floor(cycle + offset);
            const float angle = offset * DirectX::XM_2PI + phase;
            const Vector3 position =
                center + Vector3(std::cos(angle) * radius * 1.08f, radius * (-0.8f + rise * 2.2f),
                                 std::sin(angle) * radius * 1.08f);
            const Vector3 horizontal = cameraRight * (radius * 0.10f);
            const Vector3 vertical = cameraUp * (radius * 0.20f);
            const Vector3 corners[]{position + vertical, position + horizontal, position - vertical,
                                    position - horizontal};
            const float alpha = 0.58f * std::sin(rise * DirectX::XM_PI);
            for (size_t edge = 0; edge < 4; ++edge) {
                put(position, Color(0.85f, 0.60f, 1, alpha));
                put(corners[edge], Color(0.68f, 0.25f, 1, 0));
                put(corners[(edge + 1) % 4], Color(0.68f, 0.25f, 1, 0));
            }
        }
        return count;
    }
} // namespace GhostSupportAura
