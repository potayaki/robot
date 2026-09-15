#include "CAutoturret.h"
#include"CEnemy.h"
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
    m_model->SetScale(0.1f, 0.1f, 0.1f); // 自動砲台のスケールを設定
    //テクスチャディレクトリ
    std::string texDirectory = "assets/model/golfball/golf_ball.mtl";
    std::u8string modelFile = u8"assets/model/golfball/golf_ball.obj";
    //Meshを読み込む
    std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    m_model->Load(tmpStr1, texDirectory);
}

void CAutoturret::Update() {
    m_life -= 0.1f; // 寿命を減少させる処理（例として毎フレーム0.1減少）
    if (m_life <= 0.0f) {
        // 寿命が尽きた場合の処理（例: オブジェクトを削除するなど）
        Destroy();
        return;
    }

    m_model->SetPosition(m_Position);

    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();
    CEnemy* minEnemy = nullptr;
    float minDistance = 9999.0f; // 十分大きな値で初期化

    for (CEnemy*enemy : enemies) {
        float distance = (enemy->GetPosition() - m_Position).Length();

        if (distance < minDistance) {
            minDistance = minDistance;
            minEnemy = enemy;
        }
    }

    if (m_attackCooldown < 0.0f) {
        m_attackCooldown -= 1.0f; // クールダウンタイマーを減少させる
    }

    if (minEnemy != nullptr && minDistance < m_radius && m_attackCooldown < 0.0f) {
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
