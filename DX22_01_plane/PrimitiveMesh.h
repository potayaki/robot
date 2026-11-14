#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// 描画APIに依存しない、ステージ部品の頂点・法線・UV生成。
namespace PrimitiveMesh {
    enum class Shape {
        Triangle,
        Quad,
        Box,
        Sphere,
        Cone,
        Cylinder,
        Count
    };
    constexpr int DefaultSlices = 32;
    constexpr int DefaultStacks = 16;
    constexpr float Pi = 3.14159265358979323846f;

    struct Vec2 {
        float x, y;
    };

    struct Vec3 {
        float x, y, z;

        Vec3 operator-(const Vec3& b) const {
            return {x - b.x, y - b.y, z - b.z};
        }

        float Dot(const Vec3& b) const {
            return x * b.x + y * b.y + z * b.z;
        }

        Vec3 Cross(const Vec3& b) const {
            return {y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
        }
    };

    struct Vertex {
        Vec3 position, normal;
        Vec2 uv;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned> indices;
    };

    inline const char* Name(Shape shape) {
        switch (shape) {
            case Shape::Triangle:
                return "Triangle";
            case Shape::Quad:
                return "Quad";
            case Shape::Box:
                return "Box";
            case Shape::Sphere:
                return "Sphere";
            case Shape::Cone:
                return "Cone";
            case Shape::Cylinder:
                return "Cylinder";
            default:
                return "Unknown";
        }
    }

    constexpr bool IsFlat(Shape shape) {
        return shape == Shape::Triangle || shape == Shape::Quad;
    }

    namespace Detail {
        inline unsigned AddVertex(Mesh& mesh, Vec3 p, Vec3 normal, Vec2 uv) {
            mesh.vertices.push_back({p, normal, uv});
            return static_cast<unsigned>(mesh.vertices.size() - 1);
        }

        inline void Triangle(Mesh& mesh, unsigned a, unsigned b, unsigned c) {
            // 既存のGroundと同じ、外向き法線に沿う頂点順に揃える。
            const auto& va = mesh.vertices[a];
            const auto cross =
                (mesh.vertices[b].position - va.position).Cross(mesh.vertices[c].position - va.position);
            if (cross.Dot(va.normal) < 0) {
                std::swap(b, c);
            }
            mesh.indices.insert(mesh.indices.end(), {a, b, c});
        }

        inline void Face(Mesh& mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal) {
            const auto first = AddVertex(mesh, a, normal, {0, 0});
            AddVertex(mesh, b, normal, {1, 0});
            AddVertex(mesh, c, normal, {0, 1});
            AddVertex(mesh, d, normal, {1, 1});
            Triangle(mesh, first, first + 1, first + 2);
            Triangle(mesh, first + 2, first + 1, first + 3);
        }

        inline Vec2 Circle(int i, int slices) {
            // UVの継ぎ目では座標を厳密に一致させ、細い隙間を残さない。
            const float angle = 2.0f * Pi * (i == slices ? 0 : i) / slices;
            return {std::cos(angle), std::sin(angle)};
        }

        inline void Cap(Mesh& mesh, int slices, float y, float normalY) {
            const Vec3 normal{0, normalY, 0};
            const unsigned center = AddVertex(mesh, {0, y, 0}, normal, {0.5f, 0.5f});
            for (int i = 0; i <= slices; ++i) {
                const auto ring = Circle(i, slices);
                AddVertex(mesh, {0.5f * ring.x, y, 0.5f * ring.y}, normal,
                          {0.5f + 0.5f * ring.x, 0.5f - normalY * 0.5f * ring.y});
            }
            for (int i = 0; i < slices; ++i) {
                Triangle(mesh, center, center + 1 + i, center + 2 + i);
            }
        }
    } // namespace Detail

    // 基本寸法は1。板はXZ平面（上が+Y）、立体は中心が原点で高さ・直径が1。
    // UVは各面0～1。円周の継ぎ目、立方体の角、円柱の蓋は頂点を分ける。
    inline Mesh Build(Shape shape, int slices = DefaultSlices, int stacks = DefaultStacks) {
        slices = std::clamp(slices, 3, 128);
        stacks = std::clamp(stacks, 2, 64);
        Mesh mesh;
        using namespace Detail;
        switch (shape) {
            case Shape::Triangle:
                AddVertex(mesh, {0, 0, 0.5f}, {0, 1, 0}, {0.5f, 0});
                AddVertex(mesh, {0.5f, 0, -0.5f}, {0, 1, 0}, {1, 1});
                AddVertex(mesh, {-0.5f, 0, -0.5f}, {0, 1, 0}, {0, 1});
                Triangle(mesh, 0, 1, 2);
                break;
            case Shape::Quad:
                Face(mesh, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f}, {0, 1, 0});
                break;
            case Shape::Box:
                Face(mesh, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
                     {0, 1, 0});
                Face(mesh, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f},
                     {0.5f, -0.5f, 0.5f}, {0, -1, 0});
                Face(mesh, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
                     {0.5f, -0.5f, -0.5f}, {0, 0, -1});
                Face(mesh, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f},
                     {0, 0, 1});
                Face(mesh, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f},
                     {1, 0, 0});
                Face(mesh, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f},
                     {-0.5f, -0.5f, -0.5f}, {-1, 0, 0});
                break;
            case Shape::Sphere:
                for (int row = 1; row < stacks; ++row) {
                    const float latitude = Pi * row / stacks;
                    for (int i = 0; i <= slices; ++i) {
                        const auto ring = Circle(i, slices);
                        const Vec3 normal{std::sin(latitude) * ring.x, std::cos(latitude),
                                          std::sin(latitude) * ring.y};
                        AddVertex(mesh, {normal.x * 0.5f, normal.y * 0.5f, normal.z * 0.5f}, normal,
                                  {static_cast<float>(i) / slices, static_cast<float>(row) / stacks});
                    }
                }
                for (int row = 0; row < stacks - 2; ++row) {
                    for (int i = 0; i < slices; ++i) {
                        const unsigned a = row * (slices + 1) + i;
                        const unsigned b = a + slices + 1;
                        Triangle(mesh, a, a + 1, b);
                        Triangle(mesh, a + 1, b + 1, b);
                    }
                }
                for (int i = 0; i < slices; ++i) {
                    // 極は扇形ごとに分離し、ゼロ面積の三角形とUVの横断を避ける。
                    const float u = (i + 0.5f) / slices;
                    const auto top = AddVertex(mesh, {0, 0.5f, 0}, {0, 1, 0}, {u, 0});
                    Triangle(mesh, top, i, i + 1);
                    const auto bottom = AddVertex(mesh, {0, -0.5f, 0}, {0, -1, 0}, {u, 1});
                    const unsigned last = (stacks - 2) * (slices + 1) + i;
                    Triangle(mesh, bottom, last + 1, last);
                }
                break;
            case Shape::Cone:
            case Shape::Cylinder: {
                const bool cone = shape == Shape::Cone;
                const float normalScale = cone ? 1.0f / std::sqrt(1.25f) : 1.0f;
                for (int i = 0; i <= slices; ++i) {
                    const auto ring = Circle(i, slices);
                    const Vec3 normal{ring.x * normalScale, cone ? 0.5f * normalScale : 0,
                                      ring.y * normalScale};
                    AddVertex(mesh, {0.5f * ring.x, -0.5f, 0.5f * ring.y}, normal,
                              {static_cast<float>(i) / slices, 1});
                    if (!cone) {
                        AddVertex(mesh, {0.5f * ring.x, 0.5f, 0.5f * ring.y}, normal,
                                  {static_cast<float>(i) / slices, 0});
                    }
                }
                for (int i = 0; i < slices; ++i) {
                    if (cone) {
                        const float angle = 2.0f * Pi * (i + 0.5f) / slices;
                        const auto tip = AddVertex(mesh, {0, 0.5f, 0},
                                                   {std::cos(angle) * normalScale, 0.5f * normalScale,
                                                    std::sin(angle) * normalScale},
                                                   {(i + 0.5f) / slices, 0});
                        Triangle(mesh, i, i + 1, tip);
                    } else {
                        const unsigned a = 2 * i;
                        Triangle(mesh, a, a + 2, a + 1);
                        Triangle(mesh, a + 1, a + 2, a + 3);
                    }
                }
                Cap(mesh, slices, -0.5f, -1);
                if (!cone) {
                    Cap(mesh, slices, 0.5f, 1);
                }
                break;
            }
            default:
                break;
        }
        return mesh;
    }
} // namespace PrimitiveMesh
