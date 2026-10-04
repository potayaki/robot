#include "CAutoturret.h"
#include"EnemyTarget.h"
#include<vector>
#include"Game.h"
CAutoturret::CAutoturret() {
    Init();
}

CAutoturret::~CAutoturret() {
    Uninit();

}

void CAutoturret::Init() {
    m_model = new TestModel();
    m_model->Init();
    m_model->SetScale(3.0f); // 自動砲台のスケールを設定
    m_model->Load("assets/model/bullet/Bullett.fbx", "assets/model/bullet");
}

void CAutoturret::Update() {
    m_life -= 0.1f; // 寿命を減少させる処理（例として毎フレーム0.1減少）
    if (m_life <= 0.0f) {
        m_Active = false; // 寿命が尽きた場合の処理（例: オブジェクトを削除するなど）
        
        return;
    }

    m_model->SetPosition(m_Position);

    // 自動砲台もドローンを攻撃対象として認識する。
    std::vector<EnemyTarget*> enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
    EnemyTarget* minEnemy = nullptr;
    float minDistance = 9999.0f; // 十分大きな値で初期化

    for (EnemyTarget* enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead()) continue;
        float distance = (enemy->GetPosition() - m_Position).Length();

        if (distance < minDistance) {
            minDistance = distance;
            minEnemy = enemy;
        }
    }

    if (m_attackCooldown > 0.0f) {
        m_attackCooldown -= 1.0f; // クールダウンタイマーを減少させる
    }

    if (minEnemy != nullptr && minDistance < m_radius && m_attackCooldown <= 0.0f) {
        //TODO : 攻撃内容
        minEnemy->OnHit(damage);
        m_attackCooldown = 10.0f; // 攻撃後にクールダウンをリセット
    }



}

void CAutoturret::Draw(Camera* cam) {
    m_model->Draw(cam);
    //TODO : レーザー描画
}

void CAutoturret::Uninit() {
    if (m_model) {
        m_model->Uninit();
        delete m_model;
        m_model = nullptr;
    }
}
