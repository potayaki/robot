/// @file CMushroomEnemy.cpp
/// @brief 接近後に予告点滅して自爆するキノコの敵。

#include "CMushroomEnemy.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
#include "ExplosionManager.h"
#include "CForestStage.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CMushroomEnemy::Init() {
    m_model = std::make_unique<MushroomModel>();
    if (!m_model->Init()) {
        Destroy();
        return;
    }
    const Ground* ground = Ground::Current();
    m_position.y =
        (ground ? ground->HeightAt(m_position.x, m_position.z) : ForestLayout::GroundY) + m_collisionRadius;
}

void CMushroomEnemy::SetRarity(EnemyRarity rarity) {
    EnemyTarget::SetRarity(rarity);
    m_hp = EnemyVariants::Health(BaseHp, GetRarity());
    const MushroomFuse defaults;
    m_fuse.m_speed = defaults.m_speed * EnemyVariants::Get(GetRarity()).speedMultiplier;
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
    const auto previousPosition = m_position;
    Vector3 direction = player->GetPosition() - m_position;
    direction.y = 0;
    const float distance = direction.Length();
    const float travel = m_fuse.Update(GameTime::DeltaSeconds(), distance);
    if (travel > 0 && distance > 0.0001f) {
        m_position += direction * (travel / distance);
        m_rotation.y = std::atan2(direction.x, direction.z);
    }
    for (auto forest : game->GetObjects<CForestStage>()) {
        m_position = forest->ConstrainMovement(previousPosition, m_position, m_collisionRadius);
    }
    const Ground* ground = Ground::Current();
    const float groundY = ground ? ground->HeightAt(m_position.x, m_position.z) : ForestLayout::GroundY;
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
        m_model->Draw(camera, m_position - Vector3(0, m_collisionRadius, 0), m_rotation.y, m_fuse.FlashRed(),
                      1.0f, EnemyVariants::Tint(EnemySpecies::Mushroom, GetRarity()));
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
