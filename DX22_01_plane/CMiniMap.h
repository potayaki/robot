#pragma once
#include "Object.h"
#include"Texture.h"
#include"VertexBuffer.h"
#include"IndexBuffer.h"
#include"Material.h"
class CMiniMap : public Object
{
private:
    std::vector<VERTEX_3D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    VertexBuffer<VERTEX_3D> m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

    VertexBuffer<VERTEX_3D> m_VertexBufferBg;    // ：背景（枠）用
    VertexBuffer<VERTEX_3D> m_VertexBufferEnemy; // ：敵（赤色）用
    VertexBuffer<VERTEX_3D> m_VertexBufferPlayer; // ：敵（赤色）用

    Texture m_TexBackground; // レーダーの枠用
    Texture m_TexPlayer;//プレイヤー
    Texture m_TexEnemy;      // 敵の点用

    Shader m_Shader;
    std::unique_ptr<Material> m_Material;

    float m_RadarRange = 500.0f; // レーダーの探知半径

public:
    CMiniMap();
    ~CMiniMap();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

};

