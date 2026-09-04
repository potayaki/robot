#include "CPresentBox.h"
#include"Game.h"
#include"CPlayer.h"
#include<cmath>

TestModel* CPresentBox::s_sharedModel = nullptr;
int CPresentBox::s_boxCount = 0;

CPresentBox::CPresentBox() {
}

CPresentBox::~CPresentBox() {Uninit();}

void CPresentBox::Init() {
    if (s_boxCount  >= 10) {
        // 最大生成数に達している場合、モデルを取得できないため、オブジェクトを破棄する
        m_model = nullptr;
        Destroy();
        return;
    }

    s_boxCount++; // 箱の数を増やす
    m_model = s_sharedModel; // 共有モデルを使用

}

void CPresentBox::Update() {
    m_angle += 0.05f;
    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();

    if (!players.empty() && players[0] != nullptr) {

        DirectX::SimpleMath::Vector3 diff = players[0]->GetPosition() - m_Position;// プレゼントボックスとプレイヤーの位置の差を計算
        diff.y = 0;
        float hitRange = m_radius + players[0]->GetCollisionSphere().radius;

        if (diff.Length() < hitRange) {
            // プレイヤーと当たった場合の処理
            Destroy();// プレゼントボックスを非アクティブにする
            // TODO : ここでプレイヤーにアイテムを与えるなどの処理を行う
            //players[0]->heal(20); // 例: プレイヤーの体力を回復する
            return;
        }
        
    }
}

void CPresentBox::Draw(Camera* cam) {
    if (m_model) {
        m_model->SetRotation(DirectX::SimpleMath::Vector3(m_Rotation.x, m_angle, m_Rotation.z));

        float floatY = sinf(m_angle * 2.0f);
        m_model->SetScale(0.045f + floatY * 0.01f, 0.045f + floatY * 0.01f, 0.045f + floatY * 0.01f);
        float offsetY = -4.0f;

        m_model->SetPosition(DirectX::SimpleMath::Vector3(m_Position.x, m_Position.y + floatY + offsetY, m_Position.z));

        m_model->Draw(cam);
    }
}

void CPresentBox::Uninit() {

    if (m_model) {
        s_boxCount--; // 箱の数を減らす
        m_model = nullptr; // 共有モデルを解放しない
    }

}

void CPresentBox::UnloadResource() {
    if (s_sharedModel) {
        s_sharedModel->Uninit();
        delete s_sharedModel;
        s_sharedModel = nullptr;
    }

}

void CPresentBox::LoadResource() {
    if (s_sharedModel == nullptr) {
        s_sharedModel = new TestModel();
        s_sharedModel->Init();
        s_sharedModel->Load("assets/model/presentbox/TEST.fbx", "assets/model/presentbox");
    }
    s_boxCount = 0; // ステージ開始時に箱の数をリセット
}


