#pragma once
#include "Object.h"
#include"TestModel.h"
#include"Bezier.h"
class CMissile : public Object
{
private:
    TestModel* m_body;
    Bezier m_bezier;

public:
    CMissile();
    ~CMissile();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

   //誰から誰に
    void Shoot(Object& shooter, Object& target);
};

