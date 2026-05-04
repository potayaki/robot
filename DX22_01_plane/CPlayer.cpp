#include "CPlayer.h"
#include"Ground.h"
#include"Game.h"
#include"input.h"
#include"CBullet.h"
using namespace DirectX::SimpleMath;

namespace {
    const float Gravity = 0.008f;
    const float speed = 0.1f;
    const float JumpPower = 0.25f;
}

CPlayer::CPlayer() {
    m_body = nullptr;
    IsGrounded = false;
    m_velocity = { 0,0,0 };

}

CPlayer::~CPlayer() {
    Uninit();
}

void CPlayer::Init() {
    m_body = new TestCube;
    m_body->Init();
    m_body->SetScale(1.0f, 1.0f, 1.0f);



}


void CPlayer::Update() {

    m_velocity.y -= Gravity;
    m_Position += m_velocity;
    if (m_BulletTime > 0) {
        m_BulletTime--;
    }


    isGrounded();
    Move();


    m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z);
}

void CPlayer::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }

}

void CPlayer::Onland() {
    IsGrounded = true;
    m_velocity.y = 0;
}
void CPlayer::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

void CPlayer::Move() {
    if (Input::GetKeyPress(VK_A)) {
        m_Position.x -= speed;
    }
    if (Input::GetKeyPress(VK_D)) {
        m_Position.x += speed;
    }
    if (Input::GetKeyPress(VK_W)) {
        m_Position.z += speed;
    }
    if (Input::GetKeyPress(VK_S)) {
        m_Position.z -= speed;
    }

    if (Input::GetKeyTrigger(VK_SPACE) && IsGrounded) {
        m_velocity.y = JumpPower;
    }


    if (Input::GetKeyPress(VK_LBUTTON)) {
        if (m_BulletTime <= 0) {

            CBullet* bullet = Game::GetInstance()->AddObject<CBullet>();

            Vector3 bulletPos = m_Position + Vector3(0, 0, 1); // プレイヤーの前方に弾を生成
            bullet->Shoot(bulletPos, Vector3(0, 0, 1)); // 弾を前方に発射
            m_BulletTime = 60 * 0.5f; // 弾の発射後のクールダウン時間をリセット（0.5秒）
        }
    }
}

void CPlayer::isGrounded() {
    std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) {//groundsの中が空っぽかそうか１つ以上作られたか
        Ground* plane = grounds[0];
        float planeY = plane->GetPosition().y + 10.0f;//床座標の取得 + 微調整

        if (m_Position.y <= planeY) {//床に着いているか
            m_Position.y = planeY;
            Onland();
        }
        else {
            //空中状態
            IsGrounded = false;
        }

    }
}

