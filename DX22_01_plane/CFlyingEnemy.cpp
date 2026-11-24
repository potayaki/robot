/// @file CFlyingEnemy.cpp
/// @brief ホバードローンの浮遊、接近、急降下攻撃。

#include "CFlyingEnemy.h"
#include "EnemyTint.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
#include "CForestStage.h"
#include "CPresentBox.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>

using namespace DirectX::SimpleMath;

namespace {
    // モデルを追加するときは、このフォルダに Drone.fbx と参照画像を置く。
    // Swarm09のFBXは作者の環境のAlbedo.pngを参照しているため、同梱画像を明示する。
    constexpr char DroneModelFile[] = "assets/model/drone/Drone.fbx";
    constexpr char DroneTextureDirectory[] = "assets/model/drone";
    constexpr char DroneOverrideTexture[] = "assets/model/drone/albedo.tga";

    // モデルによって寸法・原点・正面軸が異なるため、ここで見た目だけ調整する。
    // 当たり判定は CFlyingEnemy.h の GetCollisionSphere() にある。
    // Swarm09の元の幅は約139。0.2倍で約28となり、半径12の判定に近い体格になる。
    constexpr float DroneModelScale = 0.2f;
    // 読み込んだ頂点の範囲の中心。原点が体から離れているため、回転前に中心を補正する。
    const Vector3 DroneModelLocalCenter(0.0f, -2.9281f, -66.2454f);
    constexpr float DroneModelOffsetY = 0.0f;
    constexpr float DroneModelPitchOffset = 0.0f;
    constexpr float DroneModelYawOffset = 0.0f;
    constexpr float DroneModelRollOffset = 0.0f;

    // 代替の簡易モデルにも種別色を適用し、通常種へ戻す場合は元の配色を再利用する。
    const std::array<Color, 9> DronePartColors = {
        Color(0.25f, 0.39f, 0.45f, 1.0f), // 機体
        Color(0.16f, 0.23f, 0.27f, 1.0f), // 横アーム
        Color(0.12f, 0.19f, 0.23f, 1.0f), Color(0.12f, 0.19f, 0.23f, 1.0f), // モーター
        Color(0.59f, 0.68f, 0.70f, 1.0f), Color(0.59f, 0.68f, 0.70f, 1.0f),
        Color(0.59f, 0.68f, 0.70f, 1.0f), Color(0.59f, 0.68f, 0.70f, 1.0f), // 回転翼
        Color(0.14f, 0.85f, 0.88f, 1.0f) // 前面センサー
    };
} // namespace

CFlyingEnemy::~CFlyingEnemy() {
    Uninit();
}

void CFlyingEnemy::Init() {
    m_hoverPhase = static_cast<float>(rand() % 628) * 0.01f;

    // FBX を配置したら、既存の TestModel 経由で読み込んで簡易ドローンを置き換える。
    if (std::filesystem::exists(DroneModelFile)) {
        m_model = std::make_unique<TestModel>();
        m_model->Init();
        m_model->Load(DroneModelFile, DroneTextureDirectory, DroneOverrideTexture, true);
        m_model->SetScale(DroneModelScale);
        return;
    }

    // 既存の白いテクスチャを着色して、機体・アーム・回転翼・前面センサーを組み立てる。
    m_partTint = EnemyTint::Acquire();
    const std::array<Vector3, PartCount> scales = {
        Vector3(0.70f, 0.28f, 0.50f),  Vector3(1.45f, 0.07f, 0.11f),  Vector3(0.27f, 0.13f, 0.27f),
        Vector3(0.27f, 0.13f, 0.27f),  Vector3(0.55f, 0.025f, 0.08f), Vector3(0.08f, 0.025f, 0.55f),
        Vector3(0.55f, 0.025f, 0.08f), Vector3(0.08f, 0.025f, 0.55f), Vector3(0.21f, 0.12f, 0.07f)};
    for (size_t i = 0; i < PartCount; ++i) {
        m_parts[i] = std::make_unique<TestCube>();
        m_parts[i]->Init();
        m_parts[i]->SetTexture("assets/texture/white.png");
        m_parts[i]->SetMaterial(DronePartColors[i]);
        m_parts[i]->SetScale(scales[i]);
    }
}

void CFlyingEnemy::SetRarity(EnemyRarity rarity) {
    EnemyTarget::SetRarity(rarity);
    m_hp = EnemyVariants::Health(BaseHp, GetRarity());
    const Color tint = EnemyVariants::Tint(EnemySpecies::Fly, GetRarity());
    for (size_t i = 0; i < m_parts.size(); ++i) {
        if (m_parts[i]) {
            const auto base = DronePartColors[i];
            const float brightness = (std::max)(0.3f, base.x * 0.299f + base.y * 0.587f + base.z * 0.114f);
            const Color colored(tint.x * brightness, tint.y * brightness, tint.z * brightness, base.w);
            m_parts[i]->SetMaterial(Color::Lerp(base, colored, tint.w));
        }
    }
}

void CFlyingEnemy::Update() {
    if (IsDead()) {
        return;
    }
    const float deltaSeconds = GameTime::DeltaSeconds();
    const float speed = EnemyVariants::Get(GetRarity()).speedMultiplier;
    GameTime::CountDown(m_attackCooldown, deltaSeconds);
    m_hoverPhase += m_hoverAngularSpeed * deltaSeconds; // 上下運動・回転翼も経過秒数に合わせる。
    m_rotorAngle += m_rotorSpeed * deltaSeconds;

    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    const Ground* ground = Ground::Current();
    if (!players.empty() && players[0] != nullptr) {
        const Vector3 previousPosition = m_position;
        CPlayer* player = players[0];
        const Vector3 playerPos = player->GetPosition();
        Vector3 toPlayer = playerPos - m_position;
        toPlayer.y = 0.0f;
        const float horizontalDistance = toPlayer.Length();
        if (horizontalDistance > 0.001f) {
            toPlayer /= horizontalDistance;
            m_rotation.y = std::atan2(toPlayer.x, toPlayer.z);
        }

        // 普段はプレイヤーから少し離れてホバリングし、間欠的に急降下する。
        if (m_diveTimer == 0 && m_attackCooldown == 0 && horizontalDistance < m_diveStartRange) {
            m_diveTimer = m_diveDuration;
            m_attackCooldown = m_diveCooldown;
        }
        if (m_diveTimer > 0) {
            GameTime::CountDown(m_diveTimer, deltaSeconds);
            m_position += toPlayer * (m_diveSpeed * speed * deltaSeconds);
            const float diveY = playerPos.y + m_diveHeightOffset;
            m_position.y += std::clamp(diveY - m_position.y, -m_verticalSpeed * speed * deltaSeconds,
                                       m_verticalSpeed * speed * deltaSeconds);
        } else {
            if (horizontalDistance > m_approachRange) {
                m_position += toPlayer * (m_approachSpeed * speed * deltaSeconds);
            }
            if (horizontalDistance < m_retreatRange) {
                m_position -= toPlayer * (m_retreatSpeed * speed * deltaSeconds);
            }
            const float groundY =
                ground ? ground->HeightAt(m_position.x, m_position.z) : ForestLayout::GroundY;
            const float hoverY = (std::max)(groundY + m_groundClearance, playerPos.y + m_playerClearance) +
                                 std::sin(m_hoverPhase) * m_hoverAmplitude;
            // 補間率も秒に換算し、低FPSでホバリングの追従が遅くならないようにする。
            const float follow = 1.0f - std::pow(0.9f, deltaSeconds * 60.0f);
            m_position.y += (hoverY - m_position.y) * follow;
        }

        for (auto forest : Game::GetInstance()->GetObjects<CForestStage>()) {
            m_position = forest->ConstrainMovement(previousPosition, m_position, GetCollisionSphere().radius);
        }
        if (ground) {
            const float minY = ground->HeightAt(m_position.x, m_position.z) + GetCollisionSphere().radius;
            m_position.y = (std::max)(m_position.y, minY);
        }

        // 急降下して触れた時だけダメージを与え、次の攻撃まで間隔を空ける。
        const float hitRange = GetCollisionSphere().radius + player->GetCollisionSphere().radius;
        if (m_diveTimer > 0 && (m_position - playerPos).LengthSquared() < hitRange * hitRange) {
            player->TakeDamage(SupportedOutgoingDamage(m_contactDamage));
            m_diveTimer = 0;
        }
    }
}

void CFlyingEnemy::Draw(Camera* cam) {
    const auto hit = HitPose();
    const Vector3 center = m_position + hit.offset;
    if (m_model) {
        const Vector3 rotation(DroneModelPitchOffset, m_rotation.y + DroneModelYawOffset,
                               DroneModelRollOffset);
        const Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
        const Vector3 centerOffset =
            Vector3::TransformNormal(DroneModelLocalCenter * DroneModelScale, rotationMatrix);
        m_model->Draw(cam, center + Vector3(0, DroneModelOffsetY, 0) - centerOffset, rotation,
                      Vector3(DroneModelScale, DroneModelScale, DroneModelScale),
                      EnemyVariants::Tint(EnemySpecies::Fly, GetRarity()), hit.flash);
        return;
    }

    // 簡易ドローンを使う場合だけ、機体の向きに合わせて回転翼を配置する。
    const Vector3 right(std::cos(m_rotation.y), 0.0f, -std::sin(m_rotation.y));
    const Vector3 forward(std::sin(m_rotation.y), 0.0f, std::cos(m_rotation.y));
    const Vector3 leftRotor = center - right * 14.0f;
    const Vector3 rightRotor = center + right * 14.0f;
    const std::array<Vector3, PartCount> positions = {center,
                                                      center,
                                                      leftRotor,
                                                      rightRotor,
                                                      leftRotor + Vector3(0.0f, 3.0f, 0.0f),
                                                      leftRotor + Vector3(0.0f, 3.0f, 0.0f),
                                                      rightRotor + Vector3(0.0f, 3.0f, 0.0f),
                                                      rightRotor + Vector3(0.0f, 3.0f, 0.0f),
                                                      center + forward * 5.6f};
    for (size_t i = 0; i < PartCount; ++i) {
        if (!m_parts[i]) {
            continue;
        }
        m_parts[i]->SetPosition(positions[i]);
        const float spin = (i >= 4 && i <= 7) ? m_rotorAngle : 0.0f;
        m_parts[i]->SetRotation(Vector3(0.0f, m_rotation.y + spin, 0.0f));
        m_parts[i]->Draw(cam, m_partTint, hit.flash);
    }
}

void CFlyingEnemy::Uninit() {
    if (m_model) {
        m_model->Uninit();
        m_model.reset();
    }
    for (auto& part : m_parts) {
        if (part) {
            part->Uninit();
            part.reset();
        }
    }
}

void CFlyingEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    // 被弾時はHPだけを減らし、ミサイル専用の爆発・画面揺れはCMissileに任せる。
    m_hp -= SupportedIncomingDamage(damage);
    if (m_hp > 0) {
        return;
    }

    Defeat(m_experience, m_dropPercent); // 飛行型にも共通の経験値・地面へのプレゼント配置を適用する。
}
