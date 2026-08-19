#pragma once
#include "Object.h"
#include"TestModel.h"
#include"Bezier.h"
class CMissile : public Object
{
private:
    TestModel* m_body;
    Bezier m_bezier;
    int damage = 3; // ミサイルのダメージ量

    bool isActive = true; // ミサイルがアクティブかどうかのフラグ

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



};

