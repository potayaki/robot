/// @brief ボスのバリアと攻撃予告の頂点を、描画地点にある地表へ合わせる。
#pragma once
#include "Ground.h"
#include <algorithm>
#include <cmath>

class GroundProjection {
    /// 描画中の地面。未配置のシーンでは呼び出し側の高さを使用する。
    const Ground* m_ground;
    /// 地面オブジェクトがない場合のワールドY座標。
    float m_flatY;

public:
    GroundProjection(const Ground* ground, float flatY) : m_ground(ground), m_flatY(flatY) {}

    DirectX::SimpleMath::Vector3 Point(DirectX::SimpleMath::Vector3 position, float lift) const {
        position.y = (m_ground ? m_ground->HeightAt(position.x, position.z) : m_flatY) + lift;
        return position;
    }

    VERTEX_3D Vertex(DirectX::SimpleMath::Vector3 position, const DirectX::SimpleMath::Color& color) const {
        VERTEX_3D vertex{};
        vertex.position = Point(position, 1.3f);
        vertex.color = color;
        vertex.uv = {0.5f, 0.5f};
        return vertex;
    }

    /// 細分した帯の上限。1区間を2枚の三角形で描く。
    static constexpr int MaxStripSegments = 64;
    static constexpr int MaxStripColumns = 5;
    static constexpr int MaxStripVertices = MaxStripSegments * MaxStripColumns * 6;

    void AppendStrip(std::vector<VERTEX_3D>& vertices, DirectX::SimpleMath::Vector3 start,
                     DirectX::SimpleMath::Vector3 end, const DirectX::SimpleMath::Vector3& halfWidth,
                     const DirectX::SimpleMath::Color& color, float endWidthScale = 1.0f) const {
        using DirectX::SimpleMath::Vector3;
        start.y = end.y = 0;
        // 長い予告線が尾根を貫いたり谷に橋を架けたりしないよう、途中でも接地させる。
        constexpr float segmentLength = 16.0f;
        const int count = std::clamp(
            static_cast<int>(std::ceil(Vector3::Distance(start, end) / segmentLength)), 1, MaxStripSegments);
        const int columns = std::clamp(static_cast<int>(std::ceil(2 * halfWidth.Length() / segmentLength)), 1,
                                       MaxStripColumns);
        for (int col = 0; col < columns; ++col) {
            const Vector3 leftOffset = halfWidth * (2.0f * col / columns - 1);
            const Vector3 rightOffset = halfWidth * (2.0f * (col + 1) / columns - 1);
            auto left = Vertex(start + leftOffset, color);
            auto right = Vertex(start + rightOffset, color);
            for (int i = 1; i <= count; ++i) {
                const float t = static_cast<float>(i) / count;
                const Vector3 center = Vector3::Lerp(start, end, t);
                const float scale = std::lerp(1.0f, endWidthScale, t);
                const auto nextLeft = Vertex(center + leftOffset * scale, color);
                const auto nextRight = Vertex(center + rightOffset * scale, color);
                vertices.insert(vertices.end(), {left, right, nextLeft, nextLeft, right, nextRight});
                left = nextLeft;
                right = nextRight;
            }
        }
    }
};
