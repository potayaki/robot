/// @file EnemyTarget.cpp
/// @brief 敵の共通インターフェースと撃破時の経験値・ドロップ処理。

#include "EnemyTarget.h"
#include "Game.h"
#include "GameSettings.h"
#include "CPlayer.h"
#include "CEmitter.h"
#include "Ground.h"
#include "CPresentBox.h"
#include "CSpanner.h"
#include "CGhostEnemy.h"
#include <algorithm>
#include <cstdlib>

namespace {
    bool HasGhostSupport(const EnemyTarget* target) {
        for (auto ghost : Game::GetInstance()->GetObjects<CGhostEnemy>()) {
            const auto targets = ghost->GetSupportTargets();
            if (std::find(targets.begin(), targets.end(), target) != targets.end()) {
                return true; // 複数のGhostが同じ相手へつないでも倍率を重ねない。
            }
        }
        return false;
    }
} // namespace

void EnemyTarget::OnDirectionalHit(int& damage, const DirectX::SimpleMath::Vector3& incoming) {
    if (!IsDead() && damage > 0) {
        m_hitReaction.Start(Game::GetInstance()->m_gameTime, incoming);
        OnHit(damage);
    }
}

EnemyHitPose EnemyTarget::HitPose() {
    return m_hitReaction.Pose(Game::GetInstance()->m_gameTime, GetCollisionSphere().radius);
}

int EnemyTarget::SupportedIncomingDamage(int damage) const {
    return HasGhostSupport(this)
               ? GhostSupport::Incoming(damage, CGhostEnemy::SupportSettings().incomingMultiplier)
               : damage;
}

int EnemyTarget::SupportedOutgoingDamage(int damage) const {
    damage = EnemyVariants::Damage(damage, GetRarity());
    return HasGhostSupport(this)
               ? GhostSupport::Outgoing(damage, CGhostEnemy::SupportSettings().outgoingMultiplier)
               : damage;
}

void EnemyTarget::Defeat(int orbDropWeight, int presentChance) {
    // 経験値オーブ10粒の抽選重みとプレゼントの抽選率は敵の種類で決まる。
    // 同じフレームの複数の弾・範囲攻撃でも、報酬を一度だけ確定する。
    if (IsDead()) {
        return;
    }
    Destroy();
    GameSettings::PlayDefeatSound();
    GameTime::PulseOnDefeat();
    const auto particles = Game::GetInstance()->GetObjects<ParticleManager>();
    if (!particles.empty()) {
        CEmitter::EmitDefeat(*particles.front(), GetCollisionSphere());
    }

    // 報酬を渡せるプレイヤーがいなければ、敵の消滅だけで処理を終える。
    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        return;
    }
    CPlayer* player = players[0];
    player->OnEnemyDefeated(m_position, EnemyVariants::OrbDropWeight(orbDropWeight, GetRarity()));

    const Ground* ground = Ground::Current();
    const bool dropSpanner = rand() % 100 < std::clamp(CSpanner::m_spawnChancePercent, 0, 100);
    if (dropSpanner) {
        auto spanner = Game::GetInstance()->AddObject<CSpanner>();
        const float groundY = ground ? ground->HeightAt(m_position.x, m_position.z) : -5.0f;
        spanner->SetPosition(m_position.x, groundY, m_position.z);
    }

    if (rand() % 100 >= EnemyVariants::PresentChance(presentChance, GetRarity())) {
        return;
    }

    // 箱の個数制限はCPresentBox::Initが担当する。ここでは生成位置を決める。
    auto present = Game::GetInstance()->AddObject<CPresentBox>();

    // 空中の敵を倒した場合も、拾える地面の高さに報酬を配置する。
    const float x = m_position.x + (dropSpanner ? 25.0f : 0.0f);
    const float groundY = ground ? ground->HeightAt(x, m_position.z) : -5.0f;
    present->SetPosition(x, groundY + 10.0f, m_position.z);
}
