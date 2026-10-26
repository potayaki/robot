#pragma once
#include "Camera.h"
#include "EyeTurretShape.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "VertexBuffer.h"

// 目型砲台の見た目だけを担当する。攻撃・寿命・標的の選択はCAutoturretで管理する。
class EyeTurretModel
{
private:
    Shader m_shader;
    VertexBuffer<VERTEX_3D> m_vertices;
    IndexBuffer m_indices;
    unsigned int m_indexCount = 0;

public:
    void Init();
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& center,
              const DirectX::SimpleMath::Quaternion& rotation);
    void Uninit();
};
