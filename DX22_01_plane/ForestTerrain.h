/// @brief 森と野営地の地表。描画・接地・照準が同じ三角形を参照する。
#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace ForestLayout {
    inline constexpr float GroundY = -5.0f;
    inline constexpr int TerrainCell = 40;
    inline constexpr int TerrainMinX = -1800;
    inline constexpr int TerrainMaxX = 1800;
    inline constexpr int TerrainMinZ = -1800;
    inline constexpr int TerrainMaxZ = 2600;

    /// 入口の尾根、草地の窪地、野営地の丘と外周の土手を格子頂点で評価する。
    inline float GroundVertexHeight(float x, float z) {
        struct Mound {
            float x, z, radiusX, radiusZ, rise;
        };

        static constexpr Mound landforms[] = {
            {-555, -965, 585, 505, 72},   {-645, -390, 320, 440, 85},   {-185, -385, 390, 365, 82},
            {-35, 65, 355, 285, -26},     {305, 305, 455, 410, 105},    {-430, 350, 325, 385, 65},
            {205, 1370, 440, 315, 78},    {65, 1810, 360, 275, -23},    {-310, 2150, 455, 290, 83},
            {545, 1800, 310, 405, 80},    {-1090, -260, 315, 670, 215}, {1150, 370, 370, 760, 285},
            {-1220, 780, 420, 430, 165},  {1240, 1320, 380, 470, 145},  {-1130, 2210, 340, 360, 170},
            {-490, -1470, 690, 390, 175}, {420, -1570, 720, 480, 225}};
        float y = GroundY;
        for (const auto& form : landforms) {
            const float dx = (x - form.x) / form.radiusX;
            const float dz = (z - form.z) / form.radiusZ;
            const float weight = (std::max)(0.0f, 1.0f - dx * dx - dz * dz);
            y += form.rise * weight * weight;
        }
        // 後景の土手も格子の端で基準面へ戻し、場外まで伸びる射線を連続にする。
        const float edge = std::clamp((z - TerrainMinZ) / 200.0f, 0.0f, 1.0f);
        return std::lerp(GroundY, y, edge * edge * (3.0f - 2.0f * edge));
    }

    struct TerrainSample {
        float height;
        // この三角形の面をセル幅だけ +X、+Z に進んだときの高低差。
        float riseX, riseZ;
    };

    /// 格子の対角線を境に、接地点が属する三角形の高さと勾配を求める。
    inline TerrainSample SampleTerrain(float x, float z) {
        if (!(x >= TerrainMinX && x < TerrainMaxX && z >= TerrainMinZ && z < TerrainMaxZ)) {
            return {GroundY, 0, 0};
        }
        constexpr int columns = (TerrainMaxX - TerrainMinX) / TerrainCell + 1;
        constexpr int rows = (TerrainMaxZ - TerrainMinZ) / TerrainCell + 1;
        static const auto heights = [] {
            std::array<std::array<float, columns>, rows> grid{};
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < columns; ++col) {
                    grid[row][col] = GroundVertexHeight(static_cast<float>(TerrainMinX + col * TerrainCell),
                                                        static_cast<float>(TerrainMinZ + row * TerrainCell));
                }
            }
            return grid;
        }();
        const float gx = (x - TerrainMinX) / TerrainCell;
        const float gz = (z - TerrainMinZ) / TerrainCell;
        const int col = (std::min)(static_cast<int>(gx), columns - 2);
        const int row = (std::min)(static_cast<int>(gz), rows - 2);
        const float u = gx - col;
        const float v = gz - row;
        const float right = heights[row][col + 1];
        const float back = heights[row + 1][col];
        if (u + v <= 1.0f) {
            const float front = heights[row][col];
            const float riseX = right - front;
            const float riseZ = back - front;
            return {front + u * riseX + v * riseZ, riseX, riseZ};
        }
        const float farCorner = heights[row + 1][col + 1];
        const float riseX = farCorner - back;
        const float riseZ = farCorner - right;
        return {farCorner + (u - 1.0f) * riseX + (v - 1.0f) * riseZ, riseX, riseZ};
    }

    inline float GroundHeight(float x, float z) {
        return SampleTerrain(x, z).height;
    }

    inline DirectX::SimpleMath::Vector3 GroundNormal(float x, float z) {
        const auto surface = SampleTerrain(x, z);
        DirectX::SimpleMath::Vector3 normal(-surface.riseX, TerrainCell, -surface.riseZ);
        normal.Normalize();
        return normal;
    }
} // namespace ForestLayout
