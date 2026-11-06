#pragma once

#include "EnemyTarget.h"
#include "TestCube.h"
#include "TestModel.h"
#include <array>
#include <memory>

// 空中で接近し、時々プレイヤーへ急降下するホバードローン。
class CFlyingEnemy : public EnemyTarget {
private:
    static constexpr size_t PartCount = 9;
    // FBX が未配置の間だけ、従来の簡易ドローンを表示する。
    std::unique_ptr<TestModel> m_model;
    std::array<std::unique_ptr<TestCube>, PartCount> m_parts;
    int m_hp = 4;
    float m_diveTimer = 0.0f; // 急降下の残り秒数。
    float m_attackCooldown = 5.0f / 3.0f; // 次の急降下までの秒数。
    float m_hoverPhase = 0.0f;
    float m_rotorAngle = 0.0f;
    int m_Droppercent = 2;//プレゼントのドロップ率
public:
    ~CFlyingEnemy() override;
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;
    void OnHit(int& damage) override;
    // モデルの大きさを変更したら、当たり判定の半径もここで調整する。
    Collision::Sphere GetCollisionSphere() override { return { m_Position, 12.0f }; }
};
