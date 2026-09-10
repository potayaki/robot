#pragma once
#include "Object.h"
#include"billboard.h"
class ExplosionManager : public Object
{
private:
    static const int POOL_SIZE = 20; // 画面に出せる最大爆発数
    billboard* m_pool[POOL_SIZE];
    bool m_active[POOL_SIZE];

public:
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // 外部から爆発を発生させる関数
    void CreateExplosion(DirectX::SimpleMath::Vector3 pos);
};

