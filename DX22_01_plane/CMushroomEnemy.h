#pragma once
#include "EnemyTarget.h"
#include "MushroomFuse.h"
#include "MushroomModel.h"

class CMushroomEnemy : public EnemyTarget
{
public:
    void Init() override;
    void Update() override;
    void Draw(Camera* camera) override;
    void Uninit() override { m_model.reset(); }
    void OnHit(int& damage) override;
    Collision::Sphere GetCollisionSphere() override { return {m_Position, 14.0f}; }
private:
    std::unique_ptr<MushroomModel> m_model;
    MushroomFuse m_fuse;
    int m_hp = 4;
    static constexpr float BlastRadius = 65.0f;
    static constexpr int BlastDamage = 8;
};
