/// @file CSpanner.cpp
/// @brief 回復スパナの出現、浮遊表示、取得時のHP回復。

#include "CSpanner.h"
#include "Game.h"
#include "CPlayer.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace DirectX::SimpleMath;

void CSpanner::LoadResource() {
    if (m_sharedModel) {
        return;
    }
    if (!std::filesystem::exists("assets/model/spanner/lod_basic_shaded.fbx")) {
        return;
    }
    m_sharedModel = std::make_unique<TestModel>();
    m_sharedModel->Init();
    m_sharedModel->Load("assets/model/spanner/lod_basic_shaded.fbx", "assets/model/spanner",
                        "assets/model/spanner/shaded.png");
}

void CSpanner::UnloadResource() {
    m_sharedModel.reset();
}

void CSpanner::Init() {
    if (!m_sharedModel || m_instanceCount >= m_maxCount) {
        Destroy();
        return;
    }
    ++m_instanceCount;
    m_counted = true;
}

void CSpanner::Update() {
    if (IsDead()) {
        return;
    }
    m_elapsed += GameTime::DeltaSeconds();
    for (auto player : Game::GetInstance()->GetObjects<CPlayer>()) {
        if (player->IsDead() || player->GetHp() <= 0 || player->GetHp() >= player->GetMaxHp()) {
            continue;
        }
        Vector3 offset = player->GetPosition() - m_position;
        offset.y = 0; // 見た目の上下動によらず、地面上の水平距離で拾えるようにする。
        const float reach = m_pickupRadius + player->GetCollisionSphere().radius;
        if (offset.LengthSquared() > reach * reach) {
            continue;
        }
        const int heal = (std::max)(1, (player->GetMaxHp() * m_healPercent + 99) / 100);
        Destroy(); // 同じフレームに複数回更新しても、回復は一度だけ。
        player->Heal(heal);
        return;
    }
}

void CSpanner::Draw(Camera* camera) {
    if (IsDead() || !m_sharedModel) {
        return;
    }
    const float pulse = std::sin(m_elapsed * DirectX::XM_2PI / m_bobPeriod);
    const float scale = m_baseScale * (1.0f + m_pulseAmount * pulse);
    const float angle = m_elapsed * m_rotationSpeed;
    // TestModelで読み込むFBX頂点から測った中心。回転しても中心位置がずれないよう補正する。
    const Vector3 center(-0.0000845f, 0.0000325f, -0.9479405f);
    const auto rotation = Matrix::CreateRotationY(angle);
    const Vector3 centerOffset = Vector3::TransformNormal(center * scale, rotation);
    m_sharedModel->SetScale(scale);
    m_sharedModel->SetRotation(Vector3(0, angle, 0));
    m_sharedModel->SetPosition(m_position + Vector3(0, m_hoverHeight + pulse * m_bobAmplitude, 0) -
                               centerOffset);
    m_sharedModel->Draw(camera);
}

void CSpanner::Uninit() {
    if (m_counted) {
        --m_instanceCount;
        m_counted = false;
    }
}
