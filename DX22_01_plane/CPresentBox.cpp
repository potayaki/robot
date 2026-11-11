/// @file CPresentBox.cpp
/// @brief 敵が落とすプレゼントの表示、取得、ランダム報酬。

#include "CPresentBox.h"
#include "Game.h"
#include "CPlayer.h"
#include <cmath>

TestModel* CPresentBox::m_sharedModel = nullptr;
int CPresentBox::m_boxCount = 0;

CPresentBox::CPresentBox() {}

CPresentBox::~CPresentBox() {
    Uninit();
}

void CPresentBox::Init() {
    if (m_boxCount >= m_maxBoxCount) {
        // 最大生成数に達している場合、モデルを取得できないため、オブジェクトを破棄する
        m_model = nullptr;
        Destroy();
        return;
    }

    m_boxCount++; // 箱の数を増やす
    m_model = m_sharedModel; // 共有モデルを使用
}

void CPresentBox::Update() {
    if (IsDead()) {
        return;
    }
    m_angle += m_rotationSpeed * GameTime::DeltaSeconds(); // ラジアン/秒。
    std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();

    if (!players.empty() && players[0] != nullptr) {
        DirectX::SimpleMath::Vector3 diff =
            players[0]->GetPosition() - m_position; // プレゼントボックスとプレイヤーの位置の差を計算
        diff.y = 0;
        float hitRange = m_radius + players[0]->GetCollisionSphere().radius;

        if (diff.Length() < hitRange) {
            // プレイヤーと当たった場合の処理
            Destroy(); // プレゼントボックスを非アクティブにする
            // レベルアップと共通の武器・スキル候補から一つを即時獲得する。
            players[0]->GivePresentReward();
            return;
        }
    }
}

void CPresentBox::Draw(Camera* cam) {
    if (m_model) {
        m_model->SetRotation(DirectX::SimpleMath::Vector3(m_rotation.x, m_angle, m_rotation.z));

        float floatY = sinf(m_angle * m_bobFrequency);
        m_model->SetScale(m_scale + floatY * m_pulseScale, m_scale + floatY * m_pulseScale,
                          m_scale + floatY * m_pulseScale);
        const float offsetY = m_modelHeightOffset;
        m_model->SetPosition(
            DirectX::SimpleMath::Vector3(m_position.x, m_position.y + floatY + offsetY, m_position.z));

        m_model->Draw(cam);
    }
}

void CPresentBox::Uninit() {
    if (m_model) {
        m_boxCount--; // 箱の数を減らす
        m_model = nullptr; // 共有モデルを解放しない
    }
}

void CPresentBox::UnloadResource() {
    if (m_sharedModel) {
        m_sharedModel->Uninit();
        delete m_sharedModel;
        m_sharedModel = nullptr;
    }
}

void CPresentBox::LoadResource() {
    if (m_sharedModel == nullptr) {
        m_sharedModel = new TestModel();
        m_sharedModel->Init();
        m_sharedModel->Load("assets/model/presentbox/TEST.fbx", "assets/model/presentbox");
    }
    m_boxCount = 0; // ステージ開始時に箱の数をリセット
}
