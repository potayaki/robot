#include "CMushroomEnemy.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
#include "ExplosionManager.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CMushroomEnemy::Init() {
    m_model = std::make_unique<MushroomModel>();
    if (!m_model->Init()) { Destroy(); return; }
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    m_Position.y = (grounds.empty() ? -5.0f : grounds[0]->GetPosition().y) + 14.0f;
    
}

void CMushroomEnemy::Update() {
    if (IsDead()) return;
    auto game = Game::GetInstance();
    const auto players = game->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) return;
    auto player = players[0];
    Vector3 direction = player->GetPosition() - m_Position;
    direction.y = 0;
    const float distance = direction.Length();
    const float travel = m_fuse.Update(GameTime::DeltaSeconds(), distance);
    if (travel > 0 && distance > 0.0001f) {
        m_Position += direction * (travel / distance);
        m_Rotation.y = std::atan2(direction.x, direction.z);
    }
    const auto grounds = game->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    // Move the collision sphere with the model. Windup always rests on the floor.
    m_Position.y = groundY + 14.0f + m_fuse.HopHeight();
    if (!m_fuse.Exploded()) return;

    // Self-destruction happens once; only weapon kills grant defeat rewards.
    Destroy();
    const auto effects = game->GetObjects<ExplosionManager>();
    if (!effects.empty()) effects[0]->CreateExplosion(m_Position);
    const auto target = player->GetCollisionSphere();
    const float reach = BlastRadius + target.radius;
    if ((target.center - m_Position).LengthSquared() <= reach * reach)
        player->TakeDamage(BlastDamage);
}

void CMushroomEnemy::Draw(Camera* camera) {
    if (m_model && !IsDead())
        m_model->Draw(camera, m_Position - Vector3(0, 14, 0), m_Rotation.y, m_fuse.FlashRed());
}

void CMushroomEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) return;
    m_hp -= damage;
    if (m_hp <= 0) Defeat(3, 10);
}
