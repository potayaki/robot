#pragma once
#ifdef _DEBUG
#include "Collision.h"
#include "Camera.h"
#include "Shader.h"
#include "VertexBuffer.h"

// 判定の中心・半径をそのまま表示するための、テクスチャを使わないデバッグ描画。
class CollisionDebugDraw
{
public:
    struct Sphere
    {
        Collision::Sphere shape;
        DirectX::SimpleMath::Color color;
    };

    void Init();
    void Draw(Camera* camera, const std::vector<Sphere>& spheres);
    void Uninit();

private:
    Shader m_shader;
    VertexBuffer<VERTEX_3D> m_vertices;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_materialBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
    unsigned m_vertexCount = 0;
    bool m_ready = false;
};
#endif
