#include "CPlayer.h"
#include"Ground.h"
#include"Game.h"
#include"input.h"
#include"CEnemy.h"
#include"CBullet.h"
#include"CMissile.h"
using namespace DirectX::SimpleMath;

namespace {
    const float Gravity = 0.008f;
    const float speed = 1.0f;
    const float JumpPower = 0.5f;
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
    if (m_currentBulletTime > 0) {
        m_currentBulletTime--;
    }
    if (m_currentMissileTime > 0) {
        m_currentMissileTime--;
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
        StartBullet();
    }

    if (Input::GetKeyPress(VK_RBUTTON)) {
        StartMissile();
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

void CPlayer::StartBullet() {
    if (m_currentBulletTime > 0)return;

    Vector3 rayOrigin;
    Vector3 Direction;

    Camera* camera = Game::GetInstance()->GetCamera();

    if (camera != nullptr) {
        camera->GetMouseRay(rayOrigin, Direction);
    }

    std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();

    if (grounds.empty()) {
        MessageBoxA(NULL, "groundがGetしていません", "確認", MB_OKCANCEL);
        return;
    }
    //床の高さ
    const float GroundY = grounds[0]->GetPosition().y;

    if (fabs(Direction.y) < 0.001f) {//床と平行ならレイがでない
        return;
    }

    const float t = (GroundY - rayOrigin.y) / Direction.y;

    if (t <= 0.0f)return;//カメラ後方は無効


    // マウスが指している床上の位置
    const Vector3 hitPosition = rayOrigin + Direction * t;

    // プレイヤー付近から弾を発射
    const Vector3 bulletStart = m_Position + Vector3(0.0f, 5.0f, 0.0f);

    Vector3 bulletDirection = hitPosition - bulletStart;
    if (bulletDirection.LengthSquared() < 0.0001f) {
        return;
    }
    bulletDirection.Normalize();

    CBullet* bullet = Game::GetInstance()->AddObject<CBullet>();
    bullet->Shoot(bulletStart, bulletDirection);

    m_currentBulletTime = m_BulletTime;

}

void CPlayer::StartMissile() {
    if (m_currentMissileTime <= 0) {

        //敵の取得
        std::vector<CEnemy*> enemys = Game::GetInstance()->GetObjects<CEnemy>();


        if (!enemys.empty()) {//敵が存在する場合
            CEnemy* target = enemys[0]; //最初の敵をターゲットにする

            //ミサイルの生成
            CMissile* missile = Game::GetInstance()->AddObject<CMissile>();


            missile->Shoot(*this, *target); //プレイヤーと敵の位置を渡してミサイルを発射

            m_currentMissileTime = m_MissileTime; // ミサイルの発射後のクールダウン時間をリセット（2秒）

        }
    }




}

