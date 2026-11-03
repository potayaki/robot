#include "CPlayer.h"
#include"Ground.h"
#include"Game.h"
#include"input.h"
#include"EnemyTarget.h"
#include"CBullet.h"
#include"CMissile.h"
#include"CAutoturret.h"
#include "CBossBattle.h"
#include"PoolManager.h"
#include "imgui.h"
#include<algorithm>
#include<cmath>
struct TargetInfo {
    EnemyTarget* enemy;
    float dotScore;//1.0に近いほど真ん中naiseki
};

using namespace DirectX::SimpleMath;

namespace {
    const float Gravity = 28.8f; // 旧0.008/フレームを加速度（距離/秒²）へ換算。
    const float speed = 60.0f; // 移動速度（距離/秒）。
    const float JumpPower = 30.0f; // ジャンプの初速（距離/秒）。

    // カメラからの照準線が敵の球に入る最初の距離を求める。
    bool RaySphereHitDistance(const Vector3& origin, const Vector3& direction,
        const Collision::Sphere& sphere, float& hitDistance) {
        const Vector3 fromCenter = origin - sphere.center;
        const float b = fromCenter.Dot(direction);
        const float c = fromCenter.LengthSquared() - sphere.radius * sphere.radius;
        const float discriminant = b * b - c; // direction は単位ベクトル。
        if (discriminant < 0.0f) return false;

        const float root = std::sqrt(discriminant);
        const float nearDistance = -b - root;
        hitDistance = nearDistance > 0.0f ? nearDistance : -b + root;
        return hitDistance > 0.0f;
    }
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


    if (m_hp <= 0) {
        return;
    }

    const float deltaSeconds = GameTime::DeltaSeconds();
    // 重力は加速度、速度は1秒あたりの移動量として積分する。
    m_Position += m_velocity * deltaSeconds;
    m_Position.y -= 0.5f * Gravity * deltaSeconds * deltaSeconds;
    m_velocity.y -= Gravity * deltaSeconds;
    GameTime::AdvanceCooldown(m_currentBulletTime, deltaSeconds);
    GameTime::AdvanceCooldown(m_currentMissileTime, deltaSeconds);


    isGrounded();
    Move();
    UpdateAutoWeapons();


    m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z);
    m_body->SetRotation(m_Rotation);
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

void CPlayer::CreateOrbs(int count) {
    // 強化時は配置を作り直し、以前のオーブを重複して残さない。
    for (COrbitalOrb* orb : m_orbs) if (orb) orb->Destroy();
    m_orbs.clear();
    float radius = 50.0f; // 回転半径
    float speed = 3.0f; // 回転スピード（ラジアン/秒）。

    for (int i = 0; i < count; ++i) {
        // 360度（XM_2PI）を個数で割って、等間隔に配置する
        float startAngle = (DirectX::XM_2PI / count) * i;

        // ゲームの世界にオーブを出現させる
        COrbitalOrb* orb = Game::GetInstance()->AddObject<COrbitalOrb>();

        // オーブにプレイヤー情報と初期設定を渡す
        if (orb != nullptr) {
            orb->SetOrbitalParam(this, startAngle, radius, speed);
            orb->SetDamage(1 + m_progression.Rank(UpgradeId::Orbit));
            m_orbs.push_back(orb); // リストに追加
        }
    }
}




void CPlayer::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

void CPlayer::Move() {

    Camera* cam = Game::GetInstance()->GetCamera();
    float angle = cam->GetCameraDirection();

    float fwdX = -sinf(angle);
    float fwdZ = -cosf(angle);
    float rightX = -cosf(angle);
    float rightZ = sinf(angle);

    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (Input::GetKeyPress(VK_W)) { moveX += fwdX; moveZ += fwdZ; }
    if (Input::GetKeyPress(VK_S)) { moveX -= fwdX; moveZ -= fwdZ; }
    if (Input::GetKeyPress(VK_D)) { moveX += rightX; moveZ += rightZ; }
    if (Input::GetKeyPress(VK_A)) { moveX -= rightX; moveZ -= rightZ; }



    DirectX::SimpleMath::Vector3 movedir(moveX, 0.0f, moveZ);

    if (movedir.LengthSquared() > 0.0f) {
        movedir.Normalize();

        const float moveSpeed = speed * (1.0f + 0.1f * m_progression.Rank(UpgradeId::MoveSpeed));
        m_Position.x += movedir.x * moveSpeed * GameTime::DeltaSeconds();
        m_Position.z += movedir.z * moveSpeed * GameTime::DeltaSeconds();

        m_Rotation.y = atan2f(movedir.x, movedir.z);

    }

    if (Input::GetKeyTrigger(VK_SPACE) && IsGrounded) {
        m_velocity.y = JumpPower;
    }


    // 射撃位置を決める前に体全体をバリアの内側へ戻す。ジャンプ中も同じ境界を使う。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>())
    {
        m_Position = battle->ConstrainPosition(m_Position, GetCollisionSphere().radius);
    }

    // 詳細画面やデバッグUIの操作クリックを射撃として扱わない。
    const bool usingMouseUI = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
    if (!usingMouseUI && Input::GetKeyPress(VK_LBUTTON)) {
        StartBullet();
    }

    if (!usingMouseUI && Input::GetKeyPress(VK_RBUTTON)) {
        StartMissile();
    }

    // オーブと砲台は獲得した時に自動で動く。F/Vキーでの無制限生成は行わない。


}

void CPlayer::isGrounded() {
    std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) {//groundsの中が空っぽかそうか１つ以上作られたか
        Ground* plane = grounds[0];
        float planeY = plane->GetPosition().y + GetCollisionSphere().radius;//床座標の取得 +半径

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
    if (m_currentBulletTime > 0) return;

    Vector3 rayOrigin;
    Vector3 direction;

    Camera* camera = Game::GetInstance()->GetCamera();
    if (camera == nullptr || !camera->GetMouseRay(rayOrigin, direction)) return;

    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (grounds.empty()) {
        return;
    }

    const float groundY = grounds[0]->GetPosition().y;
    // 地面や敵が無い方向では、弾の飛行距離に合わせた遠方を目標にする。
    constexpr float maxAimDistance = 3000.0f;
    float nearestDistance = maxAimDistance;
    Vector3 targetPosition = rayOrigin + direction * nearestDistance;

    // カメラの照準線が地面に当たる場合は、その交点を目標候補にする。
    if (direction.y < -0.001f) {
        const float groundDistance = (groundY - rayOrigin.y) / direction.y;
        if (groundDistance > 0.0f && groundDistance < nearestDistance) {
            nearestDistance = groundDistance;
            targetPosition = rayOrigin + direction * groundDistance;
        }
    }

    // 敵が地面より手前に見えている場合は敵の中心を狙い、TPSの発射位置との視差を補正する。
    // 飛行ドローンも照準線の候補に含め、プレイヤーから発射する弾の視差を補正する。
    for (EnemyTarget* enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
        if (enemy == nullptr || enemy->IsDead()) continue;
        float hitDistance = 0.0f;
        const Collision::Sphere sphere = enemy->GetCollisionSphere();
        if (RaySphereHitDistance(rayOrigin, direction, sphere, hitDistance) &&
            hitDistance < nearestDistance) {
            nearestDistance = hitDistance;
            targetPosition = sphere.center;
        }
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
        // 中央の弾は従来通り照準を狙い、追加弾だけ左右へ広げる。
        const int count = 1 + m_progression.Rank(UpgradeId::MultiShot);
        for (int i = 0; i < count; ++i) {
            const float offset = i == 0 ? 0.0f : (i % 2 ? 0.065f : -0.065f);
            Vector3 shotDirection = Vector3::TransformNormal(bulletDirection, Matrix::CreateRotationY(offset));
            CBullet* bullet = bulletManagers[0]->Spawn();
            if (bullet != nullptr) {
                bullet->SetDamage(1 + m_progression.Rank(UpgradeId::BulletPower));
                // マルチショットの各弾が、それぞれの位置から誘導先を選ぶ。
                bullet->Shoot(bulletStart, shotDirection, m_progression.Rank(UpgradeId::BulletHoming));
            }
        }
    }

    // 発射可能になった瞬間を越えた秒数も次へ持ち越し、連射間隔の丸め誤差を防ぐ。
    m_currentBulletTime += m_BulletTime;
}


void CPlayer::StartMissile() {
    if (m_currentMissileTime <= 0) {
        //敵の取得
        // ミサイルのロックオン対象にも、すべての種類の敵を含める。
        std::vector<EnemyTarget*> enemys = Game::GetInstance()->GetObjects<EnemyTarget>();
        std::vector<MissileManager*> mManagers = Game::GetInstance()->GetObjects<MissileManager>();



        if (enemys.empty() || mManagers.empty()) {
            return;
        }

        Camera* camera = Game::GetInstance()->GetCamera();
        Vector3 rayOrigin, rayDirection;
        if (camera == nullptr || !camera->GetMouseRay(rayOrigin, rayDirection)) return;
        std::vector<TargetInfo>Targets;

        for (auto& enemy : enemys) {
            if (enemy == nullptr || enemy->IsDead()) continue;
            //カメラから敵へのベクトル
            Vector3 toEnemy = enemy->GetPosition() - rayOrigin;
            toEnemy.Normalize();

            //マウスの方向と敵方向の角度の内積計算
            float dot = rayDirection.Dot(toEnemy);

            // 大型ボスは画面上の体と中心が離れるため、照準線が体の球に入る場合も対象にする。
            // 通常の小さい敵には従来の角度によるロックオン補助も残す。
            float hitDistance = 0.0f;
            if (dot > 0.99f || RaySphereHitDistance(rayOrigin, rayDirection, enemy->GetCollisionSphere(), hitDistance)) {
                Targets.push_back({ enemy,dot });
            }
        }

        if (Targets.empty()) {
            //誰もいなかった場合
            //Targets.push_back({ enemys[0],1.0f });
        }
        else {
            //真ん中に近い順番に並べる
            std::sort(Targets.begin(), Targets.end(), [](const TargetInfo& a, const TargetInfo& b) {
                return a.dotScore > b.dotScore;
        });

            std::vector<float>angles;
            switch (curRocket) {
                //最大２０個まで
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
                case 11:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 0.0f }; break;
                case 12:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f }; break;
                case 13:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 0.0f }; break;
                case 14:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f }; break;
                case 15:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.0f }; break;
                case 16:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.9375f,-0.9375f }; break;
                case 17:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.9375f,-0.9375f, 0.0f }; break;
                case 18:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.9375f,-0.9375f, 0.46875f,-0.46875f }; break;
                case 19:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.9375f,-0.9375f, 0.46875f,-0.46875f, 0.0f }; break;
                case 20:angles = { 90.0f, -90.0f, 45.0f,-45.0f, 30.0f,-30.0f, 15.0f,-15.0f, 7.5f,-7.5f, 3.75f,-3.75f, 1.875f,-1.875f, 0.9375f,-0.9375f, 0.46875f,-0.46875f, 0.234375f,-0.234375f }; break;
            }


            for (size_t i = 0; i < angles.size(); i++) {
                CMissile* missile = mManagers[0]->Spawn();
                if (missile != nullptr) {
                    // i をターゲットの数で割った余り（%）を使うことで、A→B→C→A→B... と順番に配る！
                    EnemyTarget* target = Targets[i % Targets.size()].enemy;
                    missile->SetDamage(3 + 2 * m_progression.Rank(UpgradeId::MissilePower));
                    missile->Shoot(*this, *target, angles[i]);
                }
            }



            m_currentMissileTime += m_MissileTime; // 冷却強化の端数秒も持ち越す。


        }

    }


}

