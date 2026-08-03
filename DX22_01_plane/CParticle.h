#pragma once
#include "Object.h"
#include "TestCube.h"

enum ParticleType
{
    Test,
};

class CParticle :  public Object
{
private:
    static TestCube* m_body;
    static int m_instanceCount;
    DirectX::SimpleMath::Vector3 m_velocity; // 飛んでいく方向とスピード
    float m_life;                            // 寿命（何フレームで消えるか）
    ParticleType m_type; // パーティクルの種類
public:
    CParticle();
    ~CParticle();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // 初速と寿命をセットする関数
    void SetVelocity(DirectX::SimpleMath::Vector3 vel) { m_velocity = vel; }
    void SetLife(float life) { m_life = life; }

    void SetType(ParticleType type) { m_type = type; }

    static void Preload();

};

