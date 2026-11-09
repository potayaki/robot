#include "CSpanner.h"
#include "Game.h"
#include "CPlayer.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

using namespace DirectX::SimpleMath;

void CSpanner::LoadResource()
{
    if (s_model) return;
    if (!std::filesystem::exists("assets/model/spanner/lod_basic_shaded.fbx")) return;
    s_model = std::make_unique<TestModel>();
    s_model->Init();
    s_model->Load("assets/model/spanner/lod_basic_shaded.fbx", "assets/model/spanner",
        "assets/model/spanner/shaded.png");
}



void CSpanner::UnloadResource() { s_model.reset(); }

void CSpanner::Init()
{
    if (!s_model || s_count >= m_maxCount) { Destroy(); return; }
    ++s_count;
    m_counted = true;
}

void CSpanner::Update()
{
    if (IsDead()) return;
    m_elapsed += GameTime::DeltaSeconds();
    for (auto player : Game::GetInstance()->GetObjects<CPlayer>())
    {
        if (player->IsDead() || player->GetHp() <= 0 || player->GetHp() >= player->GetMaxHp()) continue;
        Vector3 offset = player->GetPosition() - m_Position;
        offset.y = 0; // The visible wrench floats above its ground pickup position.
        const float reach = m_pickupRadius + player->GetCollisionSphere().radius;
        if (offset.LengthSquared() > reach * reach) continue;
        const int heal = (std::max)(1, (player->GetMaxHp() * m_healPercent + 99) / 100);
        Destroy(); // Consume once even if several updates run in one frame.
        player->Heal(heal);
        return;
    }
}

void CSpanner::Draw(Camera* camera)
{
    if (IsDead() || !s_model) return;
    const float pulse = std::sin(m_elapsed * DirectX::XM_2PI / 1.2f);
    const float scale = m_baseScale * (1.0f + m_pulseAmount * pulse);
    const float angle = m_elapsed * m_rotationSpeed;
    // Center measured from the FBX's raw vertices, matching TestModel's loader.
    const Vector3 center(-0.0000845f, 0.0000325f, -0.9479405f);
    const auto rotation = Matrix::CreateRotationY(angle);
    const Vector3 centerOffset = Vector3::TransformNormal(center * scale, rotation);
    s_model->SetScale(scale);
    s_model->SetRotation(Vector3(0, angle, 0));
    s_model->SetPosition(m_Position + Vector3(0, 10.0f + pulse * 2.0f, 0) - centerOffset);
    s_model->Draw(camera);
}

void CSpanner::Uninit()
{
    if (m_counted) { --s_count; m_counted = false; }
}
