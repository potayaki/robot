#pragma once
#include "Object.h"
#include "CParticle.h"
class ParticleManager : public Object
{
    private:
        static const int MaxParticles = 100; // 最大パーティクル数
        std::vector<CParticle*> m_particles; // パーティクルのリスト
public:
    ParticleManager();
    ~ParticleManager();
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    //空いているパーティクルを探して返す関数
    CParticle* Spawn();
    
};

