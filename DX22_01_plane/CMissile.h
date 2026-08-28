#pragma once
#include "Object.h"
#include"TestModel.h"
#include"Bezier.h"
#include"billboard.h"
#include"PoolManager.h"
class CMissile : public Object
{
private:
    TestModel* m_body;
    Bezier m_bezier;
    int damage = 3; // ミサイルのダメージ量

    bool isActive = true; // ミサイルがアクティブかどうかのフラグ

    float m_colRadius = 20.0f;

    Object* m_target = nullptr;

public:
    CMissile();
    ~CMissile();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    bool GetActive() { return isActive; }

    void SetActive(bool active) { isActive = active; }

   //誰から誰に
    void Shoot(Object& shooter, Object& target,float angleOffsetDebug);

    Collision::Sphere GetCollisionSphere() override {
        return { m_Position, m_colRadius };
    }

    void SetColRadius(float radius) { m_colRadius = radius; }

};

using MissileManager = PoolManager<CMissile, 25>; // 20個のミサイルを管理するプールマネージャ
using ExplosinManager = PoolManager<billboard, 16>;
