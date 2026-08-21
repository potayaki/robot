#include "CPlayer.h"
#include"Ground.h"
#include"Game.h"
#include"input.h"
#include"CEnemy.h"
#include"CBullet.h"
#include"CMissile.h"
#include"PoolManager.h"

using BulletManager = PoolManager<CBullet, 100>;
using MissileManager = PoolManager<CMissile, 20>;

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


    m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z);
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

    //Debug用のHP削減
    if (Input::GetKeyPress(VK_1)) {
        hp -= 1;
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
    /*
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

    //方向ベクトル
    Vector3 bulletDirection = hitPosition - bulletStart;
    if (bulletDirection.LengthSquared() < 0.0001f) {
        return;
    }
    bulletDirection.Normalize();

    Vector3 forward(0.0f, 0.0f, 1.0f);
    if (bulletDirection.Dot(forward) <= 0.0f) {
        return;
    }

    CBullet* bullet = Game::GetInstance()->AddObject<CBullet>();
    bullet->Shoot(bulletStart, bulletDirection);

    m_currentBulletTime = m_BulletTime;
    */
    if (m_currentBulletTime > 0) return;

    Vector3 rayOrigin;
    Vector3 Direction;

    Camera* camera = Game::GetInstance()->GetCamera();
    if (camera != nullptr) {
        camera->GetMouseRay(rayOrigin, Direction);
    }

    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (grounds.empty()) {
        return;
    }

    const float GroundY = grounds[0]->GetPosition().y;
    Vector3 targetPosition;

    // レイが下を向いていて、かつ床との交点があるかを計算
    float t = -1.0f;
    if (fabs(Direction.y) > 0.001f) {
        t = (GroundY - rayOrigin.y) / Direction.y;
    }

    if (t > 0.0f) {
        // カーソルが床に触れている場合（今まで通りの処理）
        targetPosition = rayOrigin + Direction * t;
    }
    else {
        // カーソルが空を向いている場合
        // レイの方向へ十分に遠い距離（例えば1000.0f）をターゲットにする
        float maxDistance = 1000.0f;
        targetPosition = rayOrigin + Direction * maxDistance;
    }

    // プレイヤー付近から弾を発射
    const Vector3 bulletStart = m_Position + Vector3(0.0f, 5.0f, 0.0f);

    // 弾の飛ぶ方向ベクトルを計算
    Vector3 bulletDirection = targetPosition - bulletStart;
    if (bulletDirection.LengthSquared() < 0.0001f) {
        return;
    }
    bulletDirection.Normalize();

    // 弾を生成して発射
    //CBullet* bullet = Game::GetInstance()->AddObject<CBullet>();
    //bullet->Shoot(bulletStart, bulletDirection);

    std::vector<BulletManager*> bulletManagers = Game::GetInstance()->GetObjects<BulletManager>();
    if (!bulletManagers.empty()) {
        CBullet* bullet = bulletManagers[0]->Spawn();
        if (bullet != nullptr) {
            bullet->Shoot(bulletStart, bulletDirection);
        }
    }

    m_currentBulletTime = m_BulletTime;
}


void CPlayer::StartMissile() {
    if (m_currentMissileTime <= 0) {
        //敵の取得
        std::vector<CEnemy*> enemys = Game::GetInstance()->GetObjects<CEnemy>();



        if (!enemys.empty()) {//敵が存在する場合
            CEnemy* target = enemys[0]; //最初の敵をターゲットにする
            std::vector<MissileManager*> mManagers = Game::GetInstance()->GetObjects<MissileManager>();


            if (!mManagers.empty()) {

                std::vector<float>angles;
                switch (curRocket) {
                    case 1:angles = { 0.0f }; break;
                    case 2:angles = { 90.0f, -90.0f }; break;
                    case 3:angles = { 90.0f, -90.0f, 0.0f }; break;
                    case 4:angles = { 90.0f, -90.0f, 45.0f,-45.0f }; break;
                    case 5:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 0.0f }; break;
                    case 6:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f }; break;
                    case 7:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 0.0f }; break;
                    case 8:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f }; break;
                    case 9:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 0.0f }; break;
                    case 10:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f }; break;
                }
                for (auto& a : angles) {
                    CMissile* missile = mManagers[0]->Spawn();
                    if (missile != nullptr) {
                        missile->Shoot(*this, *target, a);//プレイヤーとターゲットの敵を渡す
                    }
                }


            }



            m_currentMissileTime = m_MissileTime; // ミサイルの発射後のクールダウン時間をリセット（2秒）

        }
    }




}

