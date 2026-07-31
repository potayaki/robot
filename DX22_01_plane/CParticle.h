#pragma once
#include "Object.h"
#include "TestCube.h"
class CParticle :  public Object
{
private:
    TestCube* m_body;
    DirectX::SimpleMath::Vector3 m_velocity; // 飛んでいく方向とスピード
    float m_life;                            // 寿命（何フレームで消えるか）

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
};

