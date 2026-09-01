#pragma once
#include "Object.h"
#include"VertexBuffer.h"
#include"Texture.h"
#include"memory"
#include"Material.h"
#include"IndexBuffer.h"
class CTimer : public Object
{
private:
    std::vector<VERTEX_3D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    VertexBuffer<VERTEX_3D> m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

    Texture m_TexNumber; // number.png 用
    Shader m_Shader;
    std::unique_ptr<Material> m_Material;

public:
    CTimer();
    ~CTimer();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;
};

