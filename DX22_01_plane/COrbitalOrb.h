#pragma once
#include "Object.h"
#include"TestCube.h"
class COrbitalOrb :  public Object
{
private:
    TestCube* m_body;       // オーブの見た目
    Object* m_parentPlayer; // 中心となるプレイヤー
    float m_angle;          // 現在の角度（ラジアン）
    float m_radius;         // 回転半径
    float m_speed;          // 回転スピード

    float m_cooldownTime; // クールダウン時間
    int m_damage;         // ダメージ量

public:
    COrbitalOrb();
    ~COrbitalOrb();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // オーブとしての初期設定を行う関数
    void SetOrbitalParam(Object* player, float startAngle, float radius, float speed);

    // 当たり判定
    Collision::Sphere GetCollisionSphere() override;
};

