/// @file CPlayer.cpp
/// @brief プレイヤーの移動、照準、射撃、ロボットの姿勢更新。

#include "CPlayer.h"
#include "Ground.h"
#include "Game.h"
#include "input.h"
#include "EnemyTarget.h"
#include "CBullet.h"
#include "CMissile.h"
#include "CAutoturret.h"
#include "CBossBattle.h"
#include "AimCursor.h"
#include "PoolManager.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

struct TargetInfo {
    /// 照準候補として記録した敵。
    EnemyTarget* enemy;
    /// 照準方向との内積。1に近いほど画面の照準中心に近い。
    float dotScore;
};

using namespace DirectX::SimpleMath;

namespace {

    // カメラからの照準線が敵の球に入る最初の距離を求める。
    bool RaySphereHitDistance(const Vector3& origin, const Vector3& direction,
                              const Collision::Sphere& sphere, float& hitDistance) {
        const Vector3 fromCenter = origin - sphere.center;
        const float b = fromCenter.Dot(direction);
        const float c = fromCenter.LengthSquared() - sphere.radius * sphere.radius;
        const float discriminant = b * b - c; // direction は単位ベクトル。
        if (discriminant < 0.0f) {
            return false;
        }

        const float root = std::sqrt(discriminant);
        const float nearDistance = -b - root;
        hitDistance = nearDistance > 0.0f ? nearDistance : -b + root;
        return hitDistance > 0.0f;
    }
} // namespace

CPlayer::CPlayer() {
    m_body = nullptr;
    m_isGrounded = false;
    m_velocity = {0, 0, 0};
}

CPlayer::~CPlayer() {
    Uninit();
}

void CPlayer::Init() {
    m_body = new PlayerRobotModel;
    m_body->Init();

    // 開始時だけ上下の正面を揃える。以後、上半身はカーソルの照準へ向ける。
    const float cameraAngle = Game::GetInstance()->GetCamera()->GetCameraDirection();
    m_lowerBodyYaw = std::atan2(-std::sin(cameraAngle), -std::cos(cameraAngle));
    m_rotation.y = m_lowerBodyYaw;
    m_upperBodyPitch = 0.0f;
}

void CPlayer::Update() {
    const float deltaSeconds = GameTime::DeltaSeconds();
    if (m_hp <= 0) {
        // 操作は止めても死亡演出は更新する。開始処理はモデル側で1回だけ実行される。
        m_body->SetPosition(m_position);
        m_body->SetRotation(Vector3(0.0f, m_lowerBodyYaw, 0.0f));
        const auto grounds = Game::GetInstance()->GetObjects<Ground>();
        const float groundY =
            grounds.empty() ? m_position.y - GetCollisionSphere().radius : grounds[0]->GetPosition().y;
        m_body->BeginBreakApart(groundY);
        m_body->AdvanceBreakApart(deltaSeconds);
        m_velocity = Vector3::Zero;
        return;
    }

    // DebugのHPスライダーなどで復活した場合、残骸を片付けて通常表示へ戻す。
    m_body->RestoreBody();
    // 重力は加速度、速度は1秒あたりの移動量として積分する。
    m_position += m_velocity * deltaSeconds;
    m_position.y -= 0.5f * m_gravity * deltaSeconds * deltaSeconds;
    m_velocity.y -= m_gravity * deltaSeconds;
    GameTime::AdvanceCooldown(m_currentBulletTime, deltaSeconds);
    GameTime::AdvanceCooldown(m_currentMissileTime, deltaSeconds);

    isGrounded();
    const Vector3 previousPosition = m_position;
    Move();
    UpdateAutoWeapons();

    // 壁で止まったときは足踏みしない。ジャンプ中は待機姿勢とし、地上の歩行と区別する。
    Vector3 movement = m_position - previousPosition;
    movement.y = 0;
    const bool walking = m_isGrounded && movement.LengthSquared() > 0.000001f;
    const float animationRate =
        walking ? 1.0f + UpgradeBalance::MoveBonusPerRank * m_progression.Rank(UpgradeId::MoveSpeed) : 1.0f;
    RobotMotion displayMotion =
        walking ? SelectRobotWalkMotion(movement.x, movement.z, m_lowerBodyYaw) : RobotMotion::Idle;
    bool procedural = true;
#ifdef _DEBUG
    if (m_debugRobotMotion > 0) {
        displayMotion = static_cast<RobotMotion>(m_debugRobotMotion - 1);
    }
    procedural = m_debugRobotMotion == 0;
#endif
    // モデル全体は脚の向きに固定し、照準との差分だけを上半身の骨へ渡す。
    const float upperBodyYaw = m_rotation.y - m_lowerBodyYaw;
    m_body->SetPosition(m_position.x, m_position.y, m_position.z);
    m_body->SetRotation(Vector3(0.0f, m_lowerBodyYaw, 0.0f));
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    m_body->Advance(displayMotion, deltaSeconds, animationRate, upperBodyYaw, m_upperBodyPitch, groundY,
                    m_isGrounded && m_velocity.y <= 0.0f, procedural);
}

void CPlayer::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void CPlayer::Onland() {
    m_isGrounded = true;
    m_velocity.y = 0;
}

void CPlayer::CreateOrbs(int count) {
    // 強化時は配置を作り直し、以前のオーブを重複して残さない。
    for (COrbitalOrb* orb : m_orbs) {
        if (orb) {
            orb->Destroy();
        }
    }
    m_orbs.clear();

    for (int i = 0; i < count; ++i) {
        // 360度（XM_2PI）を個数で割って、等間隔に配置する
        float startAngle = (DirectX::XM_2PI / count) * i;

        // ゲームの世界にオーブを出現させる
        COrbitalOrb* orb = Game::GetInstance()->AddObject<COrbitalOrb>();

        // オーブにプレイヤー情報と初期設定を渡す
        if (orb != nullptr) {
            orb->SetOrbitalParam(this, startAngle, m_orbitRadius, m_orbitAngularSpeed);
            orb->SetDamage(UpgradeBalance::BaseAutoWeaponDamage + m_progression.Rank(UpgradeId::Orbit));
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

    if (Input::GetKeyPress(VK_W)) {
        moveX += fwdX;
        moveZ += fwdZ;
    }
    if (Input::GetKeyPress(VK_S)) {
        moveX -= fwdX;
        moveZ -= fwdZ;
    }
    if (Input::GetKeyPress(VK_D)) {
        moveX += rightX;
        moveZ += rightZ;
    }
    if (Input::GetKeyPress(VK_A)) {
        moveX -= rightX;
        moveZ -= rightZ;
    }

    DirectX::SimpleMath::Vector3 movedir(moveX, 0.0f, moveZ);

    if (movedir.LengthSquared() > 0.0f) {
        movedir.Normalize();

        const float moveSpeed = m_moveSpeed * (1.0f + UpgradeBalance::MoveBonusPerRank *
                                                          m_progression.Rank(UpgradeId::MoveSpeed));
        m_position.x += movedir.x * moveSpeed * GameTime::DeltaSeconds();
        m_position.z += movedir.z * moveSpeed * GameTime::DeltaSeconds();
    }

    if (Input::GetKeyTrigger(VK_SPACE) && m_isGrounded) {
        m_velocity.y = m_jumpSpeed;
    }

    // 射撃位置を決める前に体全体をバリアの内側へ戻す。ジャンプ中も同じ境界を使う。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        m_position = battle->ConstrainPosition(m_position, GetCollisionSphere().radius);
    }

    // 移動・バリア補正後の位置から毎フレーム照準を更新する。クリックしていなくても追従する。
    UpdateAim();

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
    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) { //groundsの中が空っぽかそうか１つ以上作られたか
        Ground* plane = grounds[0];
        float planeY = plane->GetPosition().y + GetCollisionSphere().radius; //床座標の取得 +半径

        if (m_position.y <= planeY) { //床に着いているか
            m_position.y = planeY;
            Onland();
        } else {
            //空中状態
            m_isGrounded = false;
        }
    }
}

bool CPlayer::TryGetAimTarget(Vector3& targetPosition) const {
    Vector3 rayOrigin;
    Vector3 direction;

    Camera* camera = Game::GetInstance()->GetCamera();
    if (camera == nullptr || !camera->GetMouseRay(rayOrigin, direction)) {
        return false;
    }

    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (grounds.empty()) {
        return false;
    }

    const float groundY = grounds[0]->GetPosition().y;
    // 地面や敵が無い方向では、弾の飛行距離に合わせた遠方を目標にする。
    constexpr float maxAimDistance = 3000.0f;
    float nearestDistance = maxAimDistance;
    targetPosition = rayOrigin + direction * nearestDistance;

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
        if (enemy == nullptr || enemy->IsDead()) {
            continue;
        }
        float hitDistance = 0.0f;
        const Collision::Sphere sphere = enemy->GetCollisionSphere();
        if (RaySphereHitDistance(rayOrigin, direction, sphere, hitDistance) &&
            hitDistance < nearestDistance) {
            nearestDistance = hitDistance;
            targetPosition = sphere.center;
        }
    }

    return true;
}

Vector3 CPlayer::GetBulletOrigin() const {
    return m_position + Vector3(0.0f, m_muzzleHeight, 0.0f);
}

void CPlayer::UpdateAim() {
    Vector3 targetPosition;
    if (!TryGetAimTarget(targetPosition)) {
        return;
    }

    // 弾と同じ発射位置から目標を見る。カメラの正面ではなく、画面内のカーソルを追う。
    const Vector3 direction = targetPosition - GetBulletOrigin();
    if (direction.LengthSquared() < 0.0001f) {
        return;
    }

    const float horizontalLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    if (horizontalLength > 0.0001f) {
        // 真上・真下を狙ったときは左右の向きを保持して、不定な旋回を防ぐ。
        m_rotation.y = std::atan2(direction.x, direction.z);
    }
    m_upperBodyPitch = std::atan2(direction.y, horizontalLength);
}

void CPlayer::StartBullet() {
    if (m_currentBulletTime > 0) {
        return;
    }

    // 上半身の向きと同じ目標計算を使い、近い敵を狙ったときの視差補正も共有する。
    Vector3 targetPosition;
    if (!TryGetAimTarget(targetPosition)) {
        return;
    }
    const Vector3 bulletStart = GetBulletOrigin();

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
            const float offset = i == 0 ? 0.0f : (i % 2 ? m_multiShotAngle : -m_multiShotAngle);
            Vector3 shotDirection =
                Vector3::TransformNormal(bulletDirection, Matrix::CreateRotationY(offset));
            CBullet* bullet = bulletManagers[0]->Spawn();
            if (bullet != nullptr) {
                bullet->SetDamage(UpgradeBalance::BaseBulletDamage +
                                  m_progression.Rank(UpgradeId::BulletPower));
                // 各弾の前方にいる候補から、画面上の照準中心に近い敵を選ぶ。
                bullet->Shoot(bulletStart, shotDirection, m_progression.Rank(UpgradeId::BulletHoming));
            }
        }
    }

    // 発射可能になった瞬間を越えた秒数も次へ持ち越し、連射間隔の丸め誤差を防ぐ。
    m_currentBulletTime += m_bulletTime;
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
        if (camera == nullptr || !camera->GetMouseRay(rayOrigin, rayDirection)) {
            return;
        }
        const auto cursor = camera->GetAimCursor();
        std::vector<TargetInfo> Targets;

        for (auto& enemy : enemys) {
            if (enemy == nullptr || enemy->IsDead() || !enemy->GetActive()) {
                continue;
            }
            const Collision::Sphere sphere = enemy->GetCollisionSphere();
            // ミサイルは大きい方の照準円を使用する。通常弾のレベル別の枠とは別判定。
            if (!cursor.Contains(sphere.center, sphere.radius)) {
                continue;
            }
            //カメラから敵へのベクトル
            Vector3 toEnemy = sphere.center - rayOrigin;
            toEnemy.Normalize();

            //マウスの方向と敵方向の角度の内積計算
            float dot = rayDirection.Dot(toEnemy);

            Targets.push_back({enemy, dot});
        }

        if (Targets.empty()) {
            //誰もいなかった場合
            //Targets.push_back({ enemys[0],1.0f });
        } else {
            //真ん中に近い順番に並べる
            std::sort(Targets.begin(), Targets.end(),
                      [](const TargetInfo& a, const TargetInfo& b) { return a.dotScore > b.dotScore; });

            std::vector<float> angles;
            // 左右対称の発射角（度）。従来の1～20発の並びを、この表と組数で表す。
            constexpr float PairedMissileAngles[] = {90.0f, 45.0f,  30.0f,   15.0f,    7.5f,
                                                     3.75f, 1.875f, 0.9375f, 0.46875f, 0.234375f};
            const int pairCount = m_currentRocketCount / 2;
            for (int pair = 0; pair < pairCount; ++pair) {
                angles.push_back(PairedMissileAngles[pair]);
                angles.push_back(-PairedMissileAngles[pair]);
            }
            if (m_currentRocketCount % 2 != 0) {
                angles.push_back(0.0f); // 奇数発の最後は正面へ発射する。
            }

            for (size_t i = 0; i < angles.size(); i++) {
                CMissile* missile = mManagers[0]->Spawn();
                if (missile != nullptr) {
                    // i をターゲットの数で割った余り（%）を使うことで、A→B→C→A→B... と順番に配る！
                    EnemyTarget* target = Targets[i % Targets.size()].enemy;
                    missile->SetDamage(UpgradeBalance::BaseMissileDamage +
                                       UpgradeBalance::MissileDamagePerRank *
                                           m_progression.Rank(UpgradeId::MissilePower));
                    missile->Shoot(*this, *target, angles[i]);
                }
            }

            m_currentMissileTime += m_missileTime; // 冷却強化の端数秒も持ち越す。
        }
    }
}
