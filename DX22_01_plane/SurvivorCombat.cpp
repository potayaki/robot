/// @file SurvivorCombat.cpp
/// @brief CPlayerのスキル取得、自動武器、回復、被ダメージ処理。

#include "CPlayer.h"
#include "Game.h"
#include "EnemyTarget.h"
#include "CAutoturret.h"
#include "CParticle.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    /// 通常の火花の大きさ。
    constexpr float SparkSize = 6.0f;
    /// 火花を表示する時間（秒）。
    constexpr float SparkLifetime = 1.0f / 3.0f;
    /// 衝撃波の円周に並べる火花の数。
    constexpr int ShockwaveSparkCount = 40;
    /// 衝撃波の火花の大きさ。
    constexpr float ShockwaveSparkSize = 10.0f;
    /// 電撃1本を火花でつなぐ区間数。
    constexpr int LightningSegments = 16;
    /// 射程内に敵がいないとき、電撃の対象を再検索する間隔（秒）。
    constexpr float LightningRetrySeconds = 0.25f;

    // 新しい自動武器には色付きの火花を使用し、ミサイル専用の爆発・画面揺れと区別する。
    void SparkAt(const Vector3& position, const Color& color, float size = SparkSize) {
        auto managers = Game::GetInstance()->GetObjects<ParticleManager>();
        if (managers.empty()) {
            return;
        }
        CParticle* particle = managers[0]->Spawn();
        particle->SetType(Spark);
        particle->SetPosition(position);
        particle->SetVelocity(Vector3::Zero);
        particle->SetColor(color);
        particle->SetScale(size);
        particle->SetLife(SparkLifetime); // 約0.33秒の火花。
    }
} // namespace

void CPlayer::OnEnemyDefeated(int experience) {
    // 敵のDefeatから呼ばれる入口。撃破数と経験値をまとめて更新する。
    ++m_kills;
    // 死亡後に残った弾の撃破でレベルアップ画面を開き、分解演出を止めないようにする。
    if (m_hp <= 0) {
        return;
    }
    const bool wasChoosing = m_progression.IsChoosing();
    m_progression.AddExperience(experience);

    // 射撃中のクリックが、そのまま報酬選択に使われないよう短い待機を設ける。
    if (!wasChoosing && m_progression.IsChoosing()) {
        m_selectionDelay = m_selectionInputDelay;
    }
}

void CPlayer::GivePresentReward() {
    // 倒れているプレイヤーがプレゼントで復活することはない。
    if (m_hp <= 0) {
        return;
    }
    ApplyUpgradeEffects(m_progression.GrantPresent(), true);
}

void CPlayer::ApplyUpgradeEffects(UpgradeId id, bool fromPresent) {
    // この関数が呼ばれる時点で、SurvivorProgression側のランク更新は完了している。
    // 通常弾・ミサイルの威力や誘導は発射時に読み取り、ここでは即時反映が必要な物を扱う。
    // 抽選側が確定したランクから性能を計算し、重複取得は同じ武器の強化として扱う。
    m_bulletTime = UpgradeBalance::BulletInterval *
                   std::pow(UpgradeBalance::ReloadMultiplier, m_progression.Rank(UpgradeId::RapidFire));
    m_missileTime = UpgradeBalance::MissileInterval *
                    std::pow(UpgradeBalance::ReloadMultiplier, m_progression.Rank(UpgradeId::MissileReload));
    m_currentBulletTime = (std::min)(m_currentBulletTime, m_bulletTime);
    m_currentMissileTime = (std::min)(m_currentMissileTime, m_missileTime);

    // 取得時だけ発生させる効果。再計算のたびにHPや弾数を増やさないようIDで分岐する。
    if (id == UpgradeId::MissileVolley) {
        SetCurRocket(m_currentRocketCount + 1);
    }
    if (id == UpgradeId::Vitality) {
        Heal(UpgradeBalance::VitalityPerRank);
    }
    if (id == UpgradeId::Heal) {
        Heal(UpgradeBalance::RecoveryHp);
    }
    if (id == UpgradeId::Orbit) {
        CreateOrbs(1 + m_progression.Rank(id));
    }

    // 初回だけ召喚し、取得済みなら同じ相棒の性能を更新する。
    if (id == UpgradeId::Turret) {
        DeployTurret();
    }

    // 初回取得時に長く待たされないよう、範囲攻撃の発動待ち時間をリセットする。
    if (id == UpgradeId::Shockwave) {
        m_shockwaveTimer = 0;
    }
    if (id == UpgradeId::Lightning) {
        m_lightningTimer = 0;
    }

    // 獲得経路だけ表示を変え、武器の強化内容は共通にする。
    m_rewardNotice = reinterpret_cast<const char*>(fromPresent ? u8"プレゼント獲得： " : u8"強化獲得： ");
    m_rewardNotice += reinterpret_cast<const char*>(GetUpgradeDefinition(id).name);
    m_rewardNoticeTime = m_rewardNoticeDuration;
}

#ifdef _DEBUG
bool CPlayer::DebugGiveUpgrade(UpgradeId id) {
    if (m_hp <= 0 || !m_progression.DebugGrantUpgrade(id)) {
        return false;
    }
    // ランクだけ変えず、オーブ生成・砲台性能・最大HPなども通常取得と同じ経路で反映する。
    ApplyUpgradeEffects(id, false);
    return true;
}
#endif

void CPlayer::Heal(int amount) {
    if (m_hp > 0 && amount > 0) {
        m_hp = (std::min)(GetMaxHp(), m_hp + amount);
    }
}

void CPlayer::TakeDamage(int damage) {
#ifdef _DEBUG
    if (m_debugInvincible) {
        return; // テスト中はダメージを受けない。既存の被弾後無敵時間とは別に扱う。
    }
#endif
    // 一時停止中や無敵時間中は、HPにも無敵時間にも変更を加えない。
    if (m_hp <= 0 || damage <= 0 || m_damageCooldown > 0 || IsGameplayPaused()) {
        return;
    }
    const int actualDamage = (std::max)(1, damage - m_progression.Rank(UpgradeId::Armor));
    m_hp = (std::max)(0, m_hp - actualDamage);

    // 敵が重なった時の同時多重ヒットを抑える。選択画面ではこの時間も進まない。
    m_damageCooldown = m_damageInvincibilitySeconds; // 被弾後、約0.33秒間は無敵。
}

Vector3 CPlayer::GetTurretFollowPosition() const {
    return EyeTurretShape::CompanionAnchor(m_position, m_rotation.y);
}

void CPlayer::DeployTurret() {
    // プールから取得して再利用するため、戦闘中に砲台のモデルを読み直さずに済む。
    auto managers = Game::GetInstance()->GetObjects<AutoTurretManager>();
    if (managers.empty() || managers[0]->GetPool().empty()) {
        return;
    }
    // 毎回Spawnすると位置・視線・攻撃待ち時間が初期化されるため、未出現時だけ呼ぶ。
    auto turret = managers[0]->GetPool().front();
    if (!turret->GetActive()) {
        turret->Spawn(GetTurretFollowPosition());
    }
    turret->ConfigureUpgrade(m_progression.Rank(UpgradeId::Turret));
}

void CPlayer::UpdateAutoWeapons() {
    // 全タイマーを秒で管理する。一時停止・スロー再生はGameTime側で反映済み。
    const float deltaSeconds = GameTime::DeltaSeconds();

    // ----- 被弾後の無敵時間と自動回復 -----
    GameTime::CountDown(m_damageCooldown, deltaSeconds);
    m_regenerationTimer += deltaSeconds;
    if (m_regenerationTimer >= UpgradeBalance::RegenerationInterval) {
        m_regenerationTimer -= UpgradeBalance::RegenerationInterval; // 3秒を越えた端数を次回へ残す。
        Heal(UpgradeBalance::RegenerationHpPerRank * m_progression.Rank(UpgradeId::Regeneration));
    }

    // ----- 自動砲台 -----
    // 死亡後にデバッグで復活した場合も、取得済みなら1体だけ復帰させる。
    const int turretRank = m_progression.Rank(UpgradeId::Turret);
    if (turretRank > 0) {
        DeployTurret();
    }

    // ----- 衝撃波：球状の範囲内にいる敵へまとめてダメージ -----
    const int shockRank = m_progression.Rank(UpgradeId::Shockwave);
    GameTime::AdvanceCooldown(m_shockwaveTimer, deltaSeconds);
    if (shockRank > 0 && m_shockwaveTimer <= 0) {
        const float radius = static_cast<float>(UpgradeBalance::ShockwaveBaseRadius +
                                                UpgradeBalance::ShockwaveRadiusPerRank * shockRank);
        int damage = UpgradeBalance::BaseAutoWeaponDamage + shockRank;

        // 敵の中心だけでなく当たり判定半径も含め、大型の敵の端にも命中させる。
        for (auto enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy->IsDead()) {
                continue;
            }
            const auto sphere = enemy->GetCollisionSphere();
            const float reach = radius + sphere.radius;
            if ((sphere.center - m_position).LengthSquared() <= reach * reach) {
                enemy->OnHit(damage);
            }
        }

        // 円周上に火花を並べ、今回の攻撃範囲を見えるようにする。
        for (int i = 0; i < ShockwaveSparkCount; ++i) {
            const float angle = DirectX::XM_2PI * i / static_cast<float>(ShockwaveSparkCount);
            SparkAt(m_position + Vector3(std::cos(angle) * radius, 0, std::sin(angle) * radius),
                    Color(0.3f, 0.7f, 1.0f, 1.0f), ShockwaveSparkSize);
        }
        m_shockwaveTimer +=
            UpgradeBalance::ShockwaveBaseInterval - UpgradeBalance::ShockwaveIntervalPerRank * shockRank;
    }

    // ----- 連鎖電撃：直前に命中した敵を起点に、次の敵を探す -----
    const int lightningRank = m_progression.Rank(UpgradeId::Lightning);
    GameTime::AdvanceCooldown(m_lightningTimer, deltaSeconds);
    if (lightningRank > 0 && m_lightningTimer <= 0) {
        auto enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
        std::vector<EnemyTarget*> struck; // 今回の電撃で命中済みの敵。同じ敵への重複を防ぐ。
        Vector3 from = m_position + Vector3(0, 5, 0);

        for (int bounce = 0; bounce < UpgradeBalance::LightningBaseTargets + lightningRank; ++bounce) {
            EnemyTarget* nearest = nullptr;

            // 最初はプレイヤーから350、2体目以降は直前の敵から140以内を探す。
            float bestDistance =
                bounce == 0 ? UpgradeBalance::LightningRange * UpgradeBalance::LightningRange
                            : UpgradeBalance::LightningChainRange * UpgradeBalance::LightningChainRange;
            for (auto enemy : enemies) {
                if (enemy->IsDead() || std::find(struck.begin(), struck.end(), enemy) != struck.end()) {
                    continue;
                }
                const float distance = (enemy->GetPosition() - from).LengthSquared();
                if (distance < bestDistance) {
                    bestDistance = distance;
                    nearest = enemy;
                }
            }

            // 届く相手がいなくなった時点で連鎖を終了する。
            if (!nearest) {
                break;
            }

            struck.push_back(nearest);
            const Vector3 to = nearest->GetPosition();

            // 起点と着弾点の間を等分し、火花で電撃の線を表現する。
            for (int i = 0; i <= LightningSegments; ++i) {
                SparkAt(Vector3::Lerp(from, to, i / static_cast<float>(LightningSegments)),
                        Color(0.7f, 0.9f, 1.0f, 1.0f));
            }
            int damage = UpgradeBalance::BaseAutoWeaponDamage + lightningRank;
            nearest->OnHit(damage);

            // 次の検索の起点を、今命中した敵の位置へ移す。
            from = to;
        }

        // 敵が射程にいない場合は短時間で再検索する。
        m_lightningTimer += struck.empty() ? LightningRetrySeconds
                                           : UpgradeBalance::LightningBaseInterval -
                                                 lightningRank / UpgradeBalance::LightningIntervalDivisor;
    }
}
