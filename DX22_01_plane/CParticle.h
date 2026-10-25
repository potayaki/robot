#pragma once
/*
パーティクルのクラス
*/
#include "Object.h"
#include "TestCube.h"

#include"Texture.h"
#include"VertexBuffer.h"
#include"IndexBuffer.h"
#include"Material.h"

#include"PoolManager.h"
enum ParticleType
{
    Test,
    Spark,// 火花
};

class CParticle :  public Object
{
private:
    static TestCube* m_body;
    static int m_instanceCount;

    //Spark用のパーティクルの情報
    static Texture* m_sparkTexture;
    static Shader* m_sparkShader;
    static VertexBuffer<VERTEX_3D>* m_sparkVB;
    static IndexBuffer* m_sparkIB;
    static Material* m_sparkMaterial;

    DirectX::SimpleMath::Vector3 m_velocity; // 飛んでいく方向とスピード
    float m_life = 1.0f; // 残りの寿命（秒）。
    ParticleType m_type = Test; // パーティクルの種類

    DirectX::SimpleMath::Color m_color; // パーティクルの色

    bool m_isActive; // パーティクルがアクティブかどうか

public:
    CParticle();
    ~CParticle();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // 初速は距離/秒、寿命は秒で指定する。
    void SetVelocity(DirectX::SimpleMath::Vector3 vel) { m_velocity = vel; }
    void SetLife(float life) { m_life = life; }

    void SetType(ParticleType type) { m_type = type; }

    void SetColor(DirectX::SimpleMath::Color color) { m_color = color; }

    bool GetActive() const { return m_isActive; }

    void SetActive(bool isActive) { m_isActive = isActive; }

    static void Preload();

};
using ParticleManager = PoolManager<CParticle, 5000>; // 1000個のパーティクルを管理するプールマネージャ

