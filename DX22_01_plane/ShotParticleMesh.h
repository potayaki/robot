/// @file ShotParticleMesh.h
/// @brief 射撃の三角形を初期化時に一つの頂点列へまとめる。

#pragma once
#include "Renderer.h"
#include <array>
#include <cmath>
#include <vector>

namespace ShotParticleMesh {
    using DirectX::SimpleMath::Vector3;

    struct Range {
        /// 共有頂点バッファ内の開始頂点と頂点数。
        UINT first = 0;
        UINT count = 0;
    };

    inline VERTEX_3D Vertex(Vector3 position, float highlight) {
        // 赤成分は縁色から芯色への補間率。テクスチャや照明を使わない。
        return {position, {}, {highlight, highlight, highlight, 1}, {}};
    }

    // X軸に沿った四角い断面と二つの先端で立体を作る。軸の真後ろからも断面が見える。
    inline void Spike(std::vector<VERTEX_3D>& vertices, Vector3 root, Vector3 widest, Vector3 tip,
                      float halfY, float halfZ, float rootLight, float bodyLight, float tipLight) {
        const std::array<Vector3, 4> rim{Vector3(0, halfY, 0), Vector3(0, 0, halfZ), Vector3(0, -halfY, 0),
                                         Vector3(0, 0, -halfZ)};
        for (size_t i = 0; i < rim.size(); ++i) {
            const auto a = Vertex(widest + rim[i], bodyLight);
            const auto b = Vertex(widest + rim[(i + 1) % rim.size()], bodyLight);
            vertices.insert(vertices.end(), {Vertex(root, rootLight), b, a, Vertex(tip, tipLight), a, b});
        }
    }

    inline Range Plume(std::vector<VERTEX_3D>& vertices) {
        const auto first = static_cast<UINT>(vertices.size());
        // 四本の短い舌を橙の外縁にし、白熱した長い芯を最後に重ねる。
        constexpr std::array<float, 4> lengths{0.65f, 0.46f, 0.56f, 0.73f};
        for (size_t i = 0; i < lengths.size(); ++i) {
            const float angle = DirectX::XM_PIDIV2 * i + 0.34f;
            const Vector3 lateral(0, std::cos(angle), std::sin(angle));
            Spike(vertices, {}, Vector3(0.20f, 0, 0) + lateral * 0.17f,
                  Vector3(lengths[i], 0, 0) + lateral * 0.5f, 0.12f, 0.12f, 0.65f, 0.32f, 0);
        }
        Spike(vertices, {}, {0.19f, 0, 0}, {1, 0, 0}, 0.23f, 0.23f, 1, 0.95f, 0.12f);
        return {first, static_cast<UINT>(vertices.size()) - first};
    }

    inline Range PlumeGlow(std::vector<VERTEX_3D>& vertices) {
        const auto first = static_cast<UINT>(vertices.size());
        // カメラを向く円盤。中心から外縁まで段階的に薄くして、板の輪郭を残さない。
        const auto glowVertex = [](Vector3 position, float opacity) {
            auto vertex = Vertex(position, 0);
            vertex.color.w = opacity;
            return vertex;
        };
        const auto center = glowVertex({}, 0.68f);
        constexpr int segments = 24;
        for (int i = 0; i < segments; ++i) {
            const float a = DirectX::XM_2PI * i / segments;
            const float b = DirectX::XM_2PI * (i + 1) / segments;
            const Vector3 p(std::cos(a), std::sin(a), 0), q(std::cos(b), std::sin(b), 0);
            const auto midA = glowVertex(p * 0.21f, 0.23f), midB = glowVertex(q * 0.21f, 0.23f);
            const auto farA = glowVertex(p * 0.36f, 0.055f), farB = glowVertex(q * 0.36f, 0.055f);
            const auto edgeA = glowVertex(p * 0.5f, 0), edgeB = glowVertex(q * 0.5f, 0);
            vertices.insert(vertices.end(), {center, midB, midA, midA, farB, farA, midA, midB, farB, farA,
                                             edgeB, edgeA, farA, farB, edgeB});
        }
        return {first, static_cast<UINT>(vertices.size()) - first};
    }

    inline Range Ribbon(std::vector<VERTEX_3D>& vertices) {
        const auto first = static_cast<UINT>(vertices.size());
        // 閉じた細い六角柱。両端の中心・断面とも実際に通った区間内に収める。
        constexpr int sides = 6;
        for (int i = 0; i < sides; ++i) {
            const float a = DirectX::XM_2PI * i / sides;
            const float b = DirectX::XM_2PI * (i + 1) / sides;
            const Vector3 aOffset(0, 0.5f * std::cos(a), 0.5f * std::sin(a));
            const Vector3 bOffset(0, 0.5f * std::cos(b), 0.5f * std::sin(b));
            const Vector3 tail(-0.5f, 0, 0), head(0.5f, 0, 0);
            const auto ta = Vertex(tail + aOffset, 0.12f), tb = Vertex(tail + bOffset, 0.12f);
            const auto ha = Vertex(head + aOffset, 0.87f), hb = Vertex(head + bOffset, 0.87f);
            vertices.insert(vertices.end(),
                            {ta, tb, hb, ta, hb, ha, Vertex(tail, 0.6f), tb, ta, Vertex(head, 1), ha, hb});
        }
        return {first, static_cast<UINT>(vertices.size()) - first};
    }

    inline Range Chip(std::vector<VERTEX_3D>& vertices) {
        const auto first = static_cast<UINT>(vertices.size());
        // 破片は重心のずれた平たい金属片。角の強い輪郭と明るい切断面を残す。
        Spike(vertices, {-0.5f, -0.15f, 0}, {0.17f, 0, 0}, {0.5f, 0.24f, 0}, 0.5f, 0.23f, 0.06f, 0.38f, 1);
        return {first, static_cast<UINT>(vertices.size()) - first};
    }

    inline Range Ring(std::vector<VERTEX_3D>& vertices) {
        const auto first = static_cast<UINT>(vertices.size());
        constexpr int segments = 24;
        for (int i = 0; i < segments; ++i) {
            // 衝撃が接点からちぎれて広がる細い弧にする。
            if (i == 2 || i == 11 || i == 17) {
                continue;
            }
            const float a = DirectX::XM_2PI * i / segments;
            const float b = DirectX::XM_2PI * (i + 1) / segments;
            const Vector3 p(std::cos(a), std::sin(a), 0), q(std::cos(b), std::sin(b), 0);
            const auto innerA = Vertex(p * 0.447f, 0.12f), innerB = Vertex(q * 0.447f, 0.12f);
            const auto outerA = Vertex(p * 0.5f, 0.82f), outerB = Vertex(q * 0.5f, 0.82f);
            vertices.insert(vertices.end(), {innerA, outerB, outerA, innerA, innerB, outerB});
        }
        return {first, static_cast<UINT>(vertices.size()) - first};
    }
} // namespace ShotParticleMesh
