#include "CPresentBox.h"
#include"Game.h"
#include"CPlayer.h"
#include<cmath>
CPresentBox::CPresentBox() {
}

CPresentBox::~CPresentBox() {Uninit();}

void CPresentBox::Init() {
    m_model = new TestModel();
    m_model->Init();
    m_model->Load("assets/model/presentbox/TEST.fbx", "assets/model/presentbox");
    m_model->SetScale(0.05f, 0.05f, 0.05f); // プレゼントボックスのサイズ
}

void CPresentBox::Update() {
    m_angle += 0.05f;
    m_model->SetRotation(DirectX::SimpleMath::Vector3(m_Rotation.x,m_angle,m_Rotation.z)); // Y軸回転

    float floatY = sinf(m_angle * 2.0f); // 上下に揺れる動き
    m_model->SetPosition(DirectX::SimpleMath::Vector3(m_Position.x, m_Position.y + floatY, m_Position.z));

    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();
    if (!players.empty() && players[0] != nullptr) {
        DirectX::SimpleMath::Vector3 diff = players[0]->GetPosition() - m_Position;
        float hitRange = m_radius + players[0]->GetCollisionSphere().radius;

        if (diff.Length() < hitRange) {
            // プレイヤーと当たった場合の処理
            Destroy();// プレゼントボックスを非アクティブにする
            // ここでプレイヤーにアイテムを与えるなどの処理を行う
            //players[0]->heal(20); // 例: プレイヤーの体力を回復する
            return;
        }

    }
}

void CPresentBox::Draw(Camera* cam) {
    if (m_model) {
        m_model->Draw(cam);
    }
}

void CPresentBox::Uninit() {
    if (m_model) {
        m_model->Uninit();
        delete m_model;
        m_model = nullptr;
    }

}


