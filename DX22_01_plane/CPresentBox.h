#pragma once
#include "Object.h"
#include"TestModel.h"

class CPresentBox :  public Object
{
private:
    TestModel* m_model;
    float m_radius = 5.0f; // 当たり判定用の半径
    float m_angle = 0.0f; // 回転角度
public:
    CPresentBox();
    ~CPresentBox();
    void Init() override;
    void Update() override;
    void Draw(Camera*cam) override;
    void Uninit() override;

    Collision::Sphere GetCollisionSphere() override {
        return Collision::Sphere{ m_Position, m_radius };
    }

};

