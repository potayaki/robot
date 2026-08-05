#pragma once
#include "Object.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
struct UIBuffer {
    float hpRate; // HPの割合（0.0f～1.0f）
    float missileRate; // ミサイルの割合（0.0f～1.0f）
    float dummy[2]; // パディング用（16バイト境界に合わせるため）
};

class CPlayerUI : public Object
{
private:
    std::vector<VERTEX_3D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    VertexBuffer<VERTEX_3D> m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    Texture m_Texture;
    Shader m_Shader;

    ID3D11Buffer* m_pUIBuffer = nullptr; // GPUに値を送るためのバッファ

    float m_hpRate;
    float m_missileRate;

public:
    CPlayerUI();
    ~CPlayerUI();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // 外から割合を設定するための関数
    void SetHpRate(float rate) { m_hpRate = rate; }
    void SetMissileRate(float rate) { m_missileRate = rate; }
};

