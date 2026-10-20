#include "CPlayer.h"
#include "Game.h"
#include "EnemyTarget.h"
#include "CAutoturret.h"
#include "CParticle.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

namespace
{
    // 新しい自動武器には色付きの火花を使用し、ミサイル専用の爆発・画面揺れと区別する。
    void SparkAt(const Vector3& position, const Color& color, float size = 6.0f)
    {
        auto managers = Game::GetInstance()->GetObjects<ParticleManager>();
        if (managers.empty())
        {
            return;
        }
        CParticle* particle = managers[0]->Spawn();
        particle->SetType(Spark);
        particle->SetPosition(position);
        particle->SetVelocity(Vector3::Zero);
        particle->SetColor(color);
        particle->SetScale(size);
        particle->SetLife(20.0f);
    }
}

void CPlayer::OnEnemyDefeated(int experience)
{
    // 敵のDefeatから呼ばれる入口。撃破数と経験値をまとめて更新する。
    ++m_kills;
    const bool wasChoosing = m_progression.IsChoosing();
    m_progression.AddExperience(experience);

    // 射撃中のクリックが、そのまま報酬選択に使われないよう短い待機を設ける。
    if (!wasChoosing && m_progression.IsChoosing())
    {
        m_selectionDelay = 12;
    }
}

void CPlayer::GivePresentReward()
{
    // 倒れているプレイヤーがプレゼントで復活することはない。
    if (m_hp <= 0)
    {
        return;
    }
    ApplyUpgradeEffects(m_progression.GrantPresent(), true);
}

void CPlayer::ApplyUpgradeEffects(UpgradeId id, bool fromPresent)
{
    // この関数が呼ばれる時点で、SurvivorProgression側のランク更新は完了している。
    // 通常弾・ミサイルの威力や誘導は発射時に読み取り、ここでは即時反映が必要な物を扱う。
    // 抽選側が確定したランクから性能を計算し、重複取得は同じ武器の強化として扱う。
    m_BulletTime = 6.0f * std::pow(0.85f, m_progression.Rank(UpgradeId::RapidFire));
    m_MissileTime = 120.0f * std::pow(0.85f, m_progression.Rank(UpgradeId::MissileReload));
    m_currentBulletTime = (std::min)(m_currentBulletTime, m_BulletTime);
    m_currentMissileTime = (std::min)(m_currentMissileTime, m_MissileTime);

    // 取得時だけ発生させる効果。再計算のたびにHPや弾数を増やさないようIDで分岐する。
    if (id == UpgradeId::MissileVolley)
    {
        SetCurRocket(curRocket + 1);
    }
    if (id == UpgradeId::Vitality)
    {
        Heal(20);
    }
    if (id == UpgradeId::Heal)
    {
        Heal(25);
    }
    if (id == UpgradeId::Orbit)
    {
        CreateOrbs(1 + m_progression.Rank(id));
    }

    // すでに出ている砲台にも強化を反映する。次の更新で新しい砲台を設置できるようにする。
    if (id == UpgradeId::Turret)
    {
        auto managers = Game::GetInstance()->GetObjects<PoolManager<CAutoturret, 3>>();
        if (!managers.empty())
        {
            for (auto turret : managers[0]->GetPool())
            {
                turret->ConfigureUpgrade(m_progression.Rank(id));
            }
        }
        m_turretTimer = 0;
    }

    // 初回取得時に長く待たされないよう、範囲攻撃の発動待ち時間をリセットする。
    if (id == UpgradeId::Shockwave)
    {
        m_shockwaveTimer = 0;
    }
    if (id == UpgradeId::Lightning)
    {
        m_lightningTimer = 0;
    }

    // 獲得経路だけ表示を変え、武器の強化内容は共通にする。
    m_rewardNotice = reinterpret_cast<const char*>(fromPresent ? u8"プレゼント獲得： " : u8"強化獲得： ");
    m_rewardNotice += reinterpret_cast<const char*>(GetUpgradeDefinition(id).name);
    m_rewardNoticeTime = 4.0f;
}

void CPlayer::Heal(int amount)
{
    if (m_hp > 0 && amount > 0)
    {
        m_hp = (std::min)(GetMaxHp(), m_hp + amount);
    }
}

void CPlayer::TakeDamage(int damage)
{
    // 一時停止中や無敵時間中は、HPにも無敵時間にも変更を加えない。
    if (m_hp <= 0 || damage <= 0 || m_damageCooldown > 0 || IsGameplayPaused())
    {
        return;
    }
    const int actualDamage = (std::max)(1, damage - m_progression.Rank(UpgradeId::Armor));
    m_hp = (std::max)(0, m_hp - actualDamage);

    // 敵が重なった時の同時多重ヒットを抑える。選択画面ではこの時間も進まない。
    m_damageCooldown = 20;
}

void CPlayer::DeployTurret()
{
    // プールから取得して再利用するため、戦闘中に砲台のモデルを読み直さずに済む。
    auto managers = Game::GetInstance()->GetObjects<PoolManager<CAutoturret, 3>>();
    if (managers.empty())
    {
        return;
    }
    auto turret = managers[0]->Spawn();
    turret->Spawn(m_Position);
    turret->ConfigureUpgrade(m_progression.Rank(UpgradeId::Turret));
}

void CPlayer::UpdateAutoWeapons()
{
    // ここで使うタイマーの単位はフレーム。60更新で1秒、180更新で3秒。
    // Game側で一時停止中のUpdateを止めるので、閲覧・選択中は時間を消費しない。

    // ----- 被弾後の無敵時間と自動回復 -----
    if (m_damageCooldown > 0)
    {
        --m_damageCooldown;
    }
    if (++m_regenerationTimer >= 180)
    {
        m_regenerationTimer = 0;
        Heal(2 * m_progression.Rank(UpgradeId::Regeneration));
    }

    // ----- 自動砲台 -----
    // 獲得した自動武器は入力なしで動作し、砲台は最大3基を循環して設置する。
    const int turretRank = m_progression.Rank(UpgradeId::Turret);
    if (turretRank > 0 && --m_turretTimer <= 0)
    {
        DeployTurret();
        m_turretTimer = 360 - 30 * (turretRank - 1);
    }

    // ----- 衝撃波：球状の範囲内にいる敵へまとめてダメージ -----
    const int shockRank = m_progression.Rank(UpgradeId::Shockwave);
    if (shockRank > 0 && --m_shockwaveTimer <= 0)
    {
        const float radius = 80.0f + 12.0f * shockRank;
        int damage = 1 + shockRank;

        // 敵の中心だけでなく当たり判定半径も含め、大型の敵の端にも命中させる。
        for (auto enemy : Game::GetInstance()->GetObjects<EnemyTarget>())
        {
            if (enemy->IsDead())
            {
                continue;
            }
            const auto sphere = enemy->GetCollisionSphere();
            const float reach = radius + sphere.radius;
            if ((sphere.center - m_Position).LengthSquared() <= reach * reach)
            {
                enemy->OnHit(damage);
            }
        }

        // 円周上に火花を並べ、今回の攻撃範囲を見えるようにする。
        for (int i = 0; i < 40; ++i)
        {
            const float angle = DirectX::XM_2PI * i / 40.0f;
            SparkAt(m_Position + Vector3(std::cos(angle) * radius, 0, std::sin(angle) * radius),
                    Color(0.3f, 0.7f, 1.0f, 1.0f),
                    10.0f);
        }
        m_shockwaveTimer = 180 - 12 * shockRank;
    }

    // ----- 連鎖電撃：直前に命中した敵を起点に、次の敵を探す -----
    const int lightningRank = m_progression.Rank(UpgradeId::Lightning);
    if (lightningRank > 0 && --m_lightningTimer <= 0)
    {
        auto enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
        std::vector<EnemyTarget*> struck; // 今回の電撃で命中済みの敵。同じ敵への重複を防ぐ。
        Vector3 from = m_Position + Vector3(0, 5, 0);

        for (int bounce = 0; bounce < 2 + lightningRank; ++bounce)
        {
            EnemyTarget* nearest = nullptr;

            // 最初はプレイヤーから350、2体目以降は直前の敵から140以内を探す。
            float bestDistance = bounce == 0 ? 350.0f * 350.0f : 140.0f * 140.0f;
            for (auto enemy : enemies)
            {
                if (enemy->IsDead() || std::find(struck.begin(), struck.end(), enemy) != struck.end())
                {
                    continue;
                }
                const float distance = (enemy->GetPosition() - from).LengthSquared();
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    nearest = enemy;
                }
            }

            // 届く相手がいなくなった時点で連鎖を終了する。
            if (!nearest)
            {
                break;
            }

            struck.push_back(nearest);
            const Vector3 to = nearest->GetPosition();

            // 起点と着弾点の間を等分し、火花で電撃の線を表現する。
            for (int i = 0; i <= 16; ++i)
            {
                SparkAt(Vector3::Lerp(from, to, i / 16.0f), Color(0.7f, 0.9f, 1.0f, 1.0f));
            }
            int damage = 1 + lightningRank;
            nearest->OnHit(damage);

            // 次の検索の起点を、今命中した敵の位置へ移す。
            from = to;
        }

        // 敵が射程にいない場合は短時間で再検索する。
        m_lightningTimer = struck.empty() ? 15 : 150 - 10 * lightningRank;
    }
}
