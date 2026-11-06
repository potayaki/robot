#include "CFlyingEnemy.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
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
}

CFlyingEnemy::~CFlyingEnemy() {
    Uninit();
}

void CFlyingEnemy::Init() {
    m_hoverPhase = static_cast<float>(rand() % 628) * 0.01f;

    // FBX を配置したら、既存の TestModel 経由で読み込んで簡易ドローンを置き換える。
    if (std::filesystem::exists(DroneModelFile)) {
        m_model = std::make_unique<TestModel>();
        m_model->Init();
        m_model->Load(DroneModelFile, DroneTextureDirectory, DroneOverrideTexture);
        m_model->SetScale(DroneModelScale);
        return;
    }

    // 既存の白いテクスチャを着色して、機体・アーム・回転翼・前面センサーを組み立てる。
    const std::array<Color, PartCount> colors = {
        Color(0.25f, 0.39f, 0.45f, 1.0f), // 機体
        Color(0.16f, 0.23f, 0.27f, 1.0f), // 横アーム
        Color(0.12f, 0.19f, 0.23f, 1.0f), Color(0.12f, 0.19f, 0.23f, 1.0f), // モーター
        Color(0.59f, 0.68f, 0.70f, 1.0f), Color(0.59f, 0.68f, 0.70f, 1.0f),
        Color(0.59f, 0.68f, 0.70f, 1.0f), Color(0.59f, 0.68f, 0.70f, 1.0f), // 回転翼
        Color(0.14f, 0.85f, 0.88f, 1.0f) // 前面センサー
    };
    const std::array<Vector3, PartCount> scales = {
        Vector3(0.70f, 0.28f, 0.50f), Vector3(1.45f, 0.07f, 0.11f),
        Vector3(0.27f, 0.13f, 0.27f), Vector3(0.27f, 0.13f, 0.27f),
        Vector3(0.55f, 0.025f, 0.08f), Vector3(0.08f, 0.025f, 0.55f),
        Vector3(0.55f, 0.025f, 0.08f), Vector3(0.08f, 0.025f, 0.55f),
        Vector3(0.21f, 0.12f, 0.07f)
    };
    for (size_t i = 0; i < PartCount; ++i) {
        m_parts[i] = std::make_unique<TestCube>();
        m_parts[i]->Init();
        m_parts[i]->SetTexture("assets/texture/white.png");
        m_parts[i]->SetMaterial(colors[i]);
        m_parts[i]->SetScale(scales[i]);
    }
}

void CFlyingEnemy::Update() {
    if (IsDead()) return;
    const float deltaSeconds = GameTime::DeltaSeconds();
    GameTime::CountDown(m_attackCooldown, deltaSeconds);
    m_hoverPhase += 2.7f * deltaSeconds; // 上下運動・回転翼も経過秒数に合わせる。
    m_rotorAngle += 27.0f * deltaSeconds;

    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    auto grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!players.empty() && players[0] != nullptr) {
        CPlayer* player = players[0];
        const Vector3 playerPos = player->GetPosition();
        Vector3 toPlayer = playerPos - m_Position;
        toPlayer.y = 0.0f;
        const float horizontalDistance = toPlayer.Length();
        if (horizontalDistance > 0.001f) {
            toPlayer /= horizontalDistance;
            m_Rotation.y = std::atan2(toPlayer.x, toPlayer.z);
        }

        // 普段はプレイヤーから少し離れてホバリングし、間欠的に急降下する。
        if (m_diveTimer == 0 && m_attackCooldown == 0 && horizontalDistance < 130.0f) {
            m_diveTimer = 55.0f / 60.0f;
            m_attackCooldown = 3.0f;
        }
        if (m_diveTimer > 0) {
            GameTime::CountDown(m_diveTimer, deltaSeconds);
            m_Position += toPlayer * (168.0f * deltaSeconds);
            const float diveY = playerPos.y + 8.0f;
            m_Position.y += std::clamp(diveY - m_Position.y, -150.0f * deltaSeconds, 150.0f * deltaSeconds);
        }
        else {
            if (horizontalDistance > 75.0f) m_Position += toPlayer * (84.0f * deltaSeconds);
            if (horizontalDistance < 50.0f) m_Position -= toPlayer * (48.0f * deltaSeconds);
            const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
            const float hoverY = (std::max)(groundY + 45.0f, playerPos.y + 35.0f)
                + std::sin(m_hoverPhase) * 3.0f;
            // 補間率も秒に換算し、低FPSでホバリングの追従が遅くならないようにする。
            const float follow = 1.0f - std::pow(0.9f, deltaSeconds * 60.0f);
            m_Position.y += (hoverY - m_Position.y) * follow;
        }

        // 急降下して触れた時だけダメージを与え、次の攻撃まで間隔を空ける。
        const float hitRange = GetCollisionSphere().radius + player->GetCollisionSphere().radius;
        if (m_diveTimer > 0 && (m_Position - playerPos).LengthSquared() < hitRange * hitRange) {
            player->TakeDamage(3);
            m_diveTimer = 0;
        }
    }

    // FBX と簡易ドローンのどちらでも、同じ飛行位置とプレイヤー方向を反映する。
    if (m_model) {
        const Vector3 rotation(DroneModelPitchOffset,
            m_Rotation.y + DroneModelYawOffset, DroneModelRollOffset);
        const Matrix rotationMatrix = Matrix::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
        // 中心のずれにも同じ拡大率・回転を適用し、向きが変わっても体と判定が離れないようにする。
        const Vector3 centerOffset = Vector3::TransformNormal(DroneModelLocalCenter * DroneModelScale, rotationMatrix);
        m_model->SetPosition(m_Position + Vector3(0, DroneModelOffsetY, 0) - centerOffset);
        m_model->SetRotation(rotation);
        return;
    }

    // 簡易ドローンを使う場合だけ、機体の向きに合わせて回転翼を配置する。
    const Vector3 right(std::cos(m_Rotation.y), 0.0f, -std::sin(m_Rotation.y));
    const Vector3 forward(std::sin(m_Rotation.y), 0.0f, std::cos(m_Rotation.y));
    const Vector3 leftRotor = m_Position - right * 14.0f;
    const Vector3 rightRotor = m_Position + right * 14.0f;
    const std::array<Vector3, PartCount> positions = {
        m_Position, m_Position, leftRotor, rightRotor,
        leftRotor + Vector3(0.0f, 3.0f, 0.0f), leftRotor + Vector3(0.0f, 3.0f, 0.0f),
        rightRotor + Vector3(0.0f, 3.0f, 0.0f), rightRotor + Vector3(0.0f, 3.0f, 0.0f),
        m_Position + forward * 5.6f
    };
    for (size_t i = 0; i < PartCount; ++i) {
        m_parts[i]->SetPosition(positions[i]);
        const float spin = (i >= 4 && i <= 7) ? m_rotorAngle : 0.0f;
        m_parts[i]->SetRotation(Vector3(0.0f, m_Rotation.y + spin, 0.0f));
    }
}

void CFlyingEnemy::Draw(Camera* cam) {
    if (m_model) {
        m_model->Draw(cam);
        return;
    }
    for (const auto& part : m_parts) {
        if (part) part->Draw(cam);
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
    if (IsDead() || damage <= 0) return;
    // 被弾時はHPだけを減らし、ミサイル専用の爆発・画面揺れはCMissileに任せる。
    m_hp -= damage;
    if (m_hp > 0) return;

    Defeat(2, m_Droppercent); // 飛行型にも共通の経験値・地面へのプレゼント配置を適用する。
}
