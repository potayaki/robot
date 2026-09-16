#pragma once
#include "Object.h"
#include"TestModel.h"
class CAutoturret : public Object
{
private:
    TestModel* m_model; // 自動砲台のモデル
    float m_life = 100.0f; // 自動砲台の寿命
    float m_attackCooldown = 10.0f; // 攻撃のクールダウン時間
    float m_radius = 500.0f; // 攻撃範囲の半径
    int damage = 1; // 攻撃のダメージ量

    bool m_Active = false; // 自動砲台がアクティブかどうかのフラグ

public:
    CAutoturret();
    ~CAutoturret();

    void Init() override; // 初期化処理
    void Update() override; // 更新処理
    void Draw(Camera* cam) override; // 描画処理
    void Uninit() override; // 終了処理(モデルの破棄)

    // 寿命を設定する関数
    void SetLife(float life) { m_life = life; }

    // 攻撃のクールダウン時間を設定する関数
    void SetAttackCooldown(float cooldown) { m_attackCooldown = cooldown; }

    // 当たり判定用の球を返す関数
    Collision::Sphere GetCollisionSphere() override {
        return { m_Position, 5.0f }; // 自動砲台の当たり判定の半径を5に設定
    }

    void Spawn(const DirectX::SimpleMath::Vector3& position) {
        m_Position = position;
        m_Active = true; // 自動砲台をアクティブにする
        m_life = 100.0f; // 寿命をリセット
        m_attackCooldown = 10.0f; // クールダウンをリセット

    }


};

