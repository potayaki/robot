/// @file CMushroomEnemy.cpp
/// @brief 接近後に予告点滅して自爆するキノコの敵。

#include "CMushroomEnemy.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
#include "ExplosionManager.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CMushroomEnemy::Init() {
    m_model = std::make_unique<MushroomModel>();
    if (!m_model->Init()) {
        Destroy();
        return;
    }
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    m_position.y = (grounds.empty() ? -5.0f : grounds[0]->GetPosition().y) + m_collisionRadius;
}

void CMushroomEnemy::Update() {
    if (IsDead()) {
        return;
    }
    auto game = Game::GetInstance();
    const auto players = game->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        return;
    }
    auto player = players[0];
    Vector3 direction = player->GetPosition() - m_position;
    direction.y = 0;
    const float distance = direction.Length();
    const float travel = m_fuse.Update(GameTime::DeltaSeconds(), distance);
    if (travel > 0 && distance > 0.0001f) {
        m_position += direction * (travel / distance);
        m_rotation.y = std::atan2(direction.x, direction.z);
    }
    const auto grounds = game->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    // 跳ねるモデルに判定球を追従させる。予備動作に入ったら地面に接地する。
    m_position.y = groundY + m_collisionRadius + m_fuse.HopHeight();
    if (!m_fuse.Exploded()) {
        return;
    }

    // 自爆は一度だけ行う。武器で倒した場合だけ撃破報酬を与える。
    const int blastDamage = SupportedOutgoingDamage(BlastDamage);
    Destroy();
    const auto effects = game->GetObjects<ExplosionManager>();
    if (!effects.empty()) {
        effects[0]->CreateExplosion(m_position);
    }
    const auto target = player->GetCollisionSphere();
    const float reach = BlastRadius + target.radius;
    if ((target.center - m_position).LengthSquared() <= reach * reach) {
        player->TakeDamage(blastDamage);
    }
}

void CMushroomEnemy::Draw(Camera* camera) {
    if (m_model && !IsDead()) {
        m_model->Draw(camera, m_position - Vector3(0, m_collisionRadius, 0), m_rotation.y, m_fuse.FlashRed());
    }
}

void CMushroomEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    m_hp -= SupportedIncomingDamage(damage);
    if (m_hp <= 0) {
        Defeat(m_experience, m_presentChance);
    }
}
