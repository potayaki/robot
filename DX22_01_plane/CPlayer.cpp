/// @file CPlayer.cpp
/// @brief プレイヤーの移動、照準、射撃、ロボットの姿勢更新。

#include "CPlayer.h"
#include "GameSettings.h"
#include "Ground.h"
#include "Game.h"
#include "input.h"
#include "EnemyTarget.h"
#include "CBullet.h"
#include "CMissile.h"
#include "CAutoturret.h"
#include "CBossBattle.h"
#include "CForestStage.h"
#include "ForestLayout.h"
#include "AimCursor.h"
#include "PoolManager.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {

    constexpr float AirControlRatio = 0.30f; // 歩行速度のうち、離陸後に操作できる割合。
    constexpr float AirControlRampSeconds = 0.30f; // 空中の操作速度がゼロから上限に達する秒数。

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
        const Ground* ground = Ground::Current();
        const float groundY = ground ? ground->HeightAt(m_position.x, m_position.z)
                                     : m_position.y - GetCollisionSphere().radius;
        m_body->BeginBreakApart(groundY);
        m_body->AdvanceBreakApart(deltaSeconds);
        m_velocity = Vector3::Zero;
        m_airControlVelocity = Vector3::Zero;
        return;
    }

    // DebugのHPスライダーなどで復活した場合、残骸を片付けて通常表示へ戻す。
    m_body->RestoreBody();
    GameTime::AdvanceCooldown(m_currentBulletTime, deltaSeconds);
    GameTime::AdvanceCooldown(m_currentMissileTime, deltaSeconds);

    const Vector3 previousPosition = m_position;
    Move();
    Vector3 aimTarget;
    const bool hasAim = TryGetAimTarget(aimTarget);
    if (hasAim) {
        UpdateAim(aimTarget);
    }

    const Ground* ground = Ground::Current();
    const float groundY = ground ? ground->HeightAt(m_position.x, m_position.z) : ForestLayout::GroundY;
    // 初速が位置に反映されるまでは足が地面にあり、踏み切る瞬間も歩行の途中の姿勢を使う。
    const bool feetOnGround = ground && m_position.y <= groundY + GetCollisionSphere().radius;
    Vector3 movement = m_position - previousPosition;
    movement.y = 0;
    const bool walking = feetOnGround && movement.LengthSquared() > 0.000001f;
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
    m_body->Advance(displayMotion, deltaSeconds, animationRate, upperBodyYaw, m_upperBodyPitch, groundY,
                    feetOnGround, procedural);

    // 今回の移動・旋回・足のIKが反映された銃口から発射する。
    const bool usingMouseUI = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
    if (!usingMouseUI && hasAim && Input::GetKeyPress(VK_LBUTTON)) {
        StartBullet(aimTarget);
    }
    if (!usingMouseUI && Input::GetKeyPress(VK_RBUTTON)) {
        StartMissile();
    }
    UpdateAutoWeapons();
}

void CPlayer::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void CPlayer::DrawSunShadow() {
    if (m_body) {
        m_body->DrawSunShadow();
    }
}

void CPlayer::Onland() {
    m_isGrounded = true;
    m_velocity = Vector3::Zero;
    m_airControlVelocity = Vector3::Zero;
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
    const Vector3 previousPosition = m_position;
    const float deltaSeconds = GameTime::DeltaSeconds();
    // ジャンプの慣性による移動も壁との衝突判定に通す。
    m_position += m_velocity * deltaSeconds;
    m_position.y -= 0.5f * m_gravity * deltaSeconds * deltaSeconds;
    m_velocity.y -= m_gravity * deltaSeconds;

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

    Vector3 walkVelocity(moveX, 0.0f, moveZ);
    if (walkVelocity.LengthSquared() > 0.0f) {
        walkVelocity.Normalize();
        const float moveSpeed = m_moveSpeed * (1.0f + UpgradeBalance::MoveBonusPerRank *
                                                          m_progression.Rank(UpgradeId::MoveSpeed));
        walkVelocity *= moveSpeed;
        if (m_isGrounded) {
            m_position += walkVelocity * deltaSeconds;
        } else {
            const Vector3 target = walkVelocity * AirControlRatio;
            Vector3 change = target - m_airControlVelocity;
            const float distance = change.Length();
            const float maxChange = moveSpeed * AirControlRatio * deltaSeconds / AirControlRampSeconds;
            if (distance > maxChange) {
                change *= maxChange / distance;
            }
            // フレーム内の平均速度で移動し、横入力を離した後も得た速度で進む。
            m_position += change * (0.5f * deltaSeconds);
            m_velocity += change;
            m_airControlVelocity += change;
        }
    }

    for (auto forest : Game::GetInstance()->GetObjects<CForestStage>()) {
        m_position = forest->ConstrainMovement(previousPosition, m_position, GetCollisionSphere().radius);
    }

    // 射撃位置を決める前に体全体をバリアの内側へ戻す。ジャンプ中も同じ境界を使う。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        m_position = battle->ConstrainPosition(m_position, GetCollisionSphere().radius);
    }
    isGrounded();
    if (Input::GetKeyTrigger(VK_SPACE) && m_isGrounded) {
        const Ground* ground = Ground::Current();
        const Vector3 normal = ground ? ground->NormalAt(m_position.x, m_position.z) : Vector3::Up;
        // 空中で操作できる分も歩行の初速に含まれる。同じキーを押し続けたときの二重加速を防ぐ。
        m_airControlVelocity = walkVelocity * AirControlRatio;
        // 歩行の水平速度と坂の勾配から踏切時の上下速度を求め、法線方向に蹴り出す。
        walkVelocity.y = -walkVelocity.Dot(normal) / normal.y;
        m_velocity = walkVelocity + normal * m_jumpSpeed;
        m_isGrounded = false;
    }
}

void CPlayer::isGrounded() {
    if (const Ground* ground = Ground::Current()) {
        const float standingY = ground->HeightAt(m_position.x, m_position.z) + GetCollisionSphere().radius;
        // 接地中は下り坂にも追従し、ジャンプ中は足元が地表に触れた時点で着地する。
        if (m_isGrounded || m_position.y <= standingY) {
            m_position.y = standingY;
            Onland();
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

    const Ground* ground = Ground::Current();
    if (!ground) {
        return false;
    }

    // 地面や敵が無い方向では、弾の飛行距離に合わせた遠方を目標にする。
    constexpr float maxAimDistance = 3000.0f;
    float nearestDistance = maxAimDistance;
    targetPosition = rayOrigin + direction * nearestDistance;

    // カメラの照準線が地面に当たる場合は、その交点を目標候補にする。
    if (const auto hit = ground->FirstHit(rayOrigin, targetPosition)) {
        nearestDistance = *hit * maxAimDistance;
        targetPosition = rayOrigin + direction * nearestDistance;
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

Vector3 CPlayer::GetAimPivot() const {
    return m_position + Vector3(0.0f, m_aimHeight, 0.0f);
}

void CPlayer::UpdateAim(const Vector3& targetPosition) {
    // 銃口を交互に使っても胴体が左右に振れないよう、機体の中心から目標を見る。
    const Vector3 direction = targetPosition - GetAimPivot();
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

void CPlayer::StartBullet(const Vector3& targetPosition) {
    if (m_currentBulletTime > 0) {
        return;
    }

    Vector3 bulletStart = GetAimPivot();
    m_body->TryGetNextMuzzle(bulletStart);

    // 弾の飛ぶ方向ベクトルを計算
    Vector3 bulletDirection = targetPosition - bulletStart;
    if (bulletDirection.LengthSquared() < 0.0001f) {
        return;
    }
    bulletDirection.Normalize();

    std::vector<BulletManager*> bulletManagers = Game::GetInstance()->GetObjects<BulletManager>();
    bool fired = false;
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
                fired = true;
            }
        }
    }

    // 多重弾でも、同じ発射位置のエフェクトは1回の射撃につき1組だけ生成する。
    if (fired) {
        m_body->OnGunFired();
        Game::GetInstance()->GetCamera()->KickForShot();
        GameSettings::PlayShotSound();
        const auto particleManagers = Game::GetInstance()->GetObjects<ParticleManager>();
        if (!particleManagers.empty()) {
            m_bulletEmitter.Emit(*particleManagers[0], bulletStart, bulletDirection);
        }
    }

    // 発射可能になった瞬間を越えた秒数も次へ持ち越し、連射間隔の丸め誤差を防ぐ。
    m_currentBulletTime += m_bulletTime;
}

std::vector<EnemyTarget*> CPlayer::GetMissileLockTargets(const AimCursor::Region& cursor,
                                                         const std::vector<EnemyTarget*>& enemies) const {
    if (m_hp <= 0) {
        return {};
    }

    struct Candidate {
        EnemyTarget* enemy;
        float alignment;
    };

    std::vector<Candidate> candidates;
    for (auto* enemy : enemies) {
        if (!enemy || enemy->IsDead() || !enemy->GetActive()) {
            continue;
        }
        const auto sphere = enemy->GetCollisionSphere();
        float alignment;
        if (cursor.TryGetMissileScore(sphere.center, sphere.radius, alignment)) {
            candidates.push_back({enemy, alignment});
        }
    }
    // 角度が同じ場合は敵の登録順を保ち、照準表示と発射先の順序を揃える。
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const Candidate& a, const Candidate& b) { return a.alignment > b.alignment; });
    const size_t count = (std::min)(candidates.size(), static_cast<size_t>(m_currentRocketCount));
    std::vector<EnemyTarget*> targets;
    targets.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        targets.push_back(candidates[i].enemy);
    }
    return targets;
}

void CPlayer::StartMissile() {
    if (m_currentMissileTime > 0) {
        return;
    }
    auto* game = Game::GetInstance();
    const auto managers = game->GetObjects<MissileManager>();
    auto* camera = game->GetCamera();
    if (managers.empty() || !camera) {
        return;
    }
    const auto targets = GetMissileLockTargets(camera->GetAimCursor(), game->GetObjects<EnemyTarget>());
    if (targets.empty()) {
        return;
    }

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

    const int blastRank = m_progression.Rank(UpgradeId::BlastExpansion);
    for (size_t i = 0; i < angles.size(); ++i) {
        CMissile* missile = managers[0]->Spawn();
        if (missile != nullptr) {
            // 敵より弾数が多いときは、照準中心に近い敵から順番に繰り返し配る。
            EnemyTarget* target = targets[i % targets.size()];
            missile->SetDamage(UpgradeBalance::MissileDamage(m_progression.Rank(UpgradeId::MissilePower)));
            missile->Shoot(*this, *target, angles[i], blastRank);
        }
    }
    m_currentMissileTime += m_missileTime; // 冷却強化の端数秒も持ち越す。
}
