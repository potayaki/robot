#include "CPlayer.h"
#include "Game.h"
#include "EnemyTarget.h"
#include "CAutoturret.h"
#include "CParticle.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;
namespace {
    // 新しい自動武器には色付きの火花を使用し、ミサイル専用の爆発・画面揺れと区別する。
    void SparkAt(const Vector3& position, const Color& color, float size = 6.0f) {
        auto managers = Game::GetInstance()->GetObjects<ParticleManager>();
        if (managers.empty()) return;
        CParticle* particle = managers[0]->Spawn();
        particle->SetType(Spark);
        particle->SetPosition(position);
        particle->SetVelocity(Vector3::Zero);
        particle->SetColor(color);
        particle->SetScale(size);
        particle->SetLife(20.0f);
    }
}

void CPlayer::OnEnemyDefeated(int experience) {
    ++m_kills;
    const bool wasChoosing = m_progression.IsChoosing();
    m_progression.AddExperience(experience);
    // 射撃中のクリックが、そのまま報酬選択に使われないよう短い待機を設ける。
    if (!wasChoosing && m_progression.IsChoosing()) m_selectionDelay = 12;
}

void CPlayer::GivePresentReward() {
    if (m_hp <= 0) return;
    ApplyUpgradeEffects(m_progression.GrantPresent(), true);
}

void CPlayer::ApplyUpgradeEffects(UpgradeId id, bool fromPresent) {
    // 抽選側が確定したランクから性能を計算し、重複取得は同じ武器の強化として扱う。
    m_BulletTime = 6.0f * std::pow(0.85f, m_progression.Rank(UpgradeId::RapidFire));
    m_MissileTime = 120.0f * std::pow(0.85f, m_progression.Rank(UpgradeId::MissileReload));
    m_currentBulletTime = (std::min)(m_currentBulletTime, m_BulletTime);
    m_currentMissileTime = (std::min)(m_currentMissileTime, m_MissileTime);
    if (id == UpgradeId::MissileVolley) SetCurRocket(curRocket + 1);
    if (id == UpgradeId::Vitality) Heal(20);
    if (id == UpgradeId::Heal) Heal(25);
    if (id == UpgradeId::Orbit) CreateOrbs(1 + m_progression.Rank(id));
    if (id == UpgradeId::Turret) {
        auto managers = Game::GetInstance()->GetObjects<PoolManager<CAutoturret, 3>>();
        if (!managers.empty()) {
            for (auto turret : managers[0]->GetPool()) turret->ConfigureUpgrade(m_progression.Rank(id));
        }
        m_turretTimer = 0;
    }
    if (id == UpgradeId::Shockwave) m_shockwaveTimer = 0;
    if (id == UpgradeId::Lightning) m_lightningTimer = 0;
    m_rewardNotice = reinterpret_cast<const char*>(fromPresent ? u8"プレゼント獲得： " : u8"強化獲得： ");
    m_rewardNotice += reinterpret_cast<const char*>(GetUpgradeDefinition(id).name);
    m_rewardNoticeTime = 4.0f;
}

void CPlayer::Heal(int amount) {
    if (m_hp > 0 && amount > 0) m_hp = (std::min)(GetMaxHp(), m_hp + amount);
}

void CPlayer::TakeDamage(int damage) {
    if (m_hp <= 0 || damage <= 0 || m_damageCooldown > 0 || IsGameplayPaused()) return;
    const int actualDamage = (std::max)(1, damage - m_progression.Rank(UpgradeId::Armor));
    m_hp = (std::max)(0, m_hp - actualDamage);
    // 敵が重なった時の同時多重ヒットを抑える。選択画面ではこの時間も進まない。
    m_damageCooldown = 20;
}

void CPlayer::DeployTurret() {
    auto managers = Game::GetInstance()->GetObjects<PoolManager<CAutoturret, 3>>();
    if (managers.empty()) return;
    auto turret = managers[0]->Spawn();
    turret->Spawn(m_Position);
    turret->ConfigureUpgrade(m_progression.Rank(UpgradeId::Turret));
}

void CPlayer::UpdateAutoWeapons() {
    if (m_damageCooldown > 0) --m_damageCooldown;
    if (++m_regenerationTimer >= 180) {
        m_regenerationTimer = 0;
        Heal(2 * m_progression.Rank(UpgradeId::Regeneration));
    }
    // 獲得した自動武器は入力なしで動作し、砲台は最大3基を循環して設置する。
    const int turretRank = m_progression.Rank(UpgradeId::Turret);
    if (turretRank > 0 && --m_turretTimer <= 0) {
        DeployTurret();
        m_turretTimer = 360 - 30 * (turretRank - 1);
    }
    const int shockRank = m_progression.Rank(UpgradeId::Shockwave);
    if (shockRank > 0 && --m_shockwaveTimer <= 0) {
        const float radius = 80.0f + 12.0f * shockRank;
        int damage = 1 + shockRank;
        for (auto enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy->IsDead()) continue;
            const auto sphere = enemy->GetCollisionSphere();
            const float reach = radius + sphere.radius;
            if ((sphere.center - m_Position).LengthSquared() <= reach * reach) enemy->OnHit(damage);
        }
        for (int i = 0; i < 40; ++i) {
            const float angle = DirectX::XM_2PI * i / 40.0f;
            SparkAt(m_Position + Vector3(std::cos(angle) * radius, 0, std::sin(angle) * radius),
                Color(0.3f, 0.7f, 1.0f, 1.0f), 10.0f);
        }
        m_shockwaveTimer = 180 - 12 * shockRank;
    }
    const int lightningRank = m_progression.Rank(UpgradeId::Lightning);
    if (lightningRank > 0 && --m_lightningTimer <= 0) {
        auto enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
        std::vector<EnemyTarget*> struck;
        Vector3 from = m_Position + Vector3(0, 5, 0);
        for (int bounce = 0; bounce < 2 + lightningRank; ++bounce) {
            EnemyTarget* nearest = nullptr;
            float bestDistance = bounce == 0 ? 350.0f * 350.0f : 140.0f * 140.0f;
            for (auto enemy : enemies) {
                if (enemy->IsDead() || std::find(struck.begin(), struck.end(), enemy) != struck.end()) continue;
                const float distance = (enemy->GetPosition() - from).LengthSquared();
                if (distance < bestDistance) { bestDistance = distance; nearest = enemy; }
            }
            if (!nearest) break;
            struck.push_back(nearest);
            const Vector3 to = nearest->GetPosition();
            for (int i = 0; i <= 16; ++i) {
                SparkAt(Vector3::Lerp(from, to, i / 16.0f), Color(0.7f, 0.9f, 1.0f, 1.0f));
            }
            int damage = 1 + lightningRank;
            nearest->OnHit(damage);
            from = to;
        }
        // 敵が射程にいない場合は短時間で再検索する。
        m_lightningTimer = struck.empty() ? 15 : 150 - 10 * lightningRank;
    }
}
