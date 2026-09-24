#include "COrbitalOrb.h"
#include <DirectXMath.h>

COrbitalOrb::COrbitalOrb() {
    m_body = nullptr;
    m_parentPlayer = nullptr;
    m_angle = 0.0f;
    m_radius = 5.0f;
    m_speed = 0.05f; // 毎フレームの回転スピード
}

COrbitalOrb::~COrbitalOrb() {
    Uninit();
}

void COrbitalOrb::Init() {
    m_body = new TestCube();
    m_body->Init();
    // プレイヤー(1.0f)より少し小さくする
    m_body->SetScale(0.5f, 0.5f, 0.5f);
}

void COrbitalOrb::SetOrbitalParam(Object* player, float startAngle, float radius, float speed) {
    m_parentPlayer = player;
    m_angle = startAngle;
    m_radius = radius;
    m_speed = speed;
}

void COrbitalOrb::Update() {
    // 基準となるプレイヤーがいなくなったら自身も消滅させる
    if (m_parentPlayer == nullptr || m_parentPlayer->IsDead()) {
        Destroy();
        return;
    }

    // 角度の更新（ラジアン）
    m_angle += m_speed;
    if (m_angle > DirectX::XM_2PI) {
        m_angle -= DirectX::XM_2PI;
    }

    // プレイヤーの現在位置を取得
    DirectX::SimpleMath::Vector3 playerPos = m_parentPlayer->GetPosition();

    // プレイヤーの周囲を回るXZ座標を計算
    m_Position.x = playerPos.x + m_radius * cosf(m_angle);
    m_Position.y = playerPos.y + 1.0f; // プレイヤーの少し上に浮かす
    m_Position.z = playerPos.z + m_radius * sinf(m_angle);

    // 見た目のモデルに座標を適用
    if (m_body) {
        m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z);
    }
}

void COrbitalOrb::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void COrbitalOrb::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

Collision::Sphere COrbitalOrb::GetCollisionSphere() {
    // 独自の当たり判定サイズを返す（敵との接触判定用）
    return { m_Position, 2.0f };
}
