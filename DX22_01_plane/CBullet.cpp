/// @file CBullet.cpp
/// @brief 通常弾の発射、移動、誘導、移動区間での命中判定。

#include "CBullet.h"
#include "Game.h"
#include "GameSettings.h"
#include "EnemyTarget.h"
#include "Collision.h"
#include "CEmitter.h"
#include "Ground.h"
#include "PoolManager.h"
#include "BulletHoming.h"
#include "AimCursor.h"
#include "CPlayer.h"
#include <cmath>

namespace {
    // 実際に通過した2点で発光帯を張る。速度欄は描画用の単位方向。
    bool SetRibbon(CParticle& ribbon, const Vector3& from, const Vector3& to, float lifeSeconds) {
        const Vector3 offset = to - from;
        const float length = offset.Length();
        if (length <= 0.0001f) {
            return false;
        }
        constexpr float width = 1.8f;
        ribbon.SetType(BulletRibbon);
        ribbon.SetPosition((from + to) * 0.5f);
        ribbon.SetVelocity(offset / length);
        ribbon.SetScale(Vector3(length, width, 1.0f));
        ribbon.SetColor(Color(0.48f, 0.84f, 1.0f, 0.88f));
        ribbon.SetLife(lifeSeconds);
        return true;
    }
} // namespace

void CBullet::Init() {
    static std::weak_ptr<TestModel> cachedBulletModel;
    m_sharedModel =
        TestModel::AcquireShared(cachedBulletModel, "assets/model/bullet/Bullett.fbx", "assets/model/bullet");
}

void CBullet::Update() {
    // ----- 1. 寿命の確認 -----
    // 弾は削除せず非アクティブにして、プールから次の発射時に再利用する。
    if (m_remainingLifeSeconds <= 0.0f) {
        SetActive(false);
        return;
    }

    // 寿命が切れる最後の更新では、残っている時間の分だけ移動する。
    const float deltaSeconds = (std::min)(GameTime::DeltaSeconds(), m_remainingLifeSeconds);

    // 軌跡の補間と衝突判定に使う、このフレームの移動開始位置。
    const Vector3 previousPosition = m_position;

    // ----- 2. 誘導先の検索と方向の更新 -----
    // この敵リストは、後半の衝突判定でも共通で使用する。
    const auto enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
    // 強化取得の直後も、表示中の枠の大きさと飛翔中の弾の誘導範囲を揃える。
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (!players.empty()) {
        m_homingRank = players[0]->GetBulletHomingRank();
    }
    if (m_homingRank > 0) {
        // 弾用カーソルの枠内で、画面上のカーソル中心に最も近い敵を選び直す。
        // 弾に近い横の敵へ引かれず、通過後は枠内に残った別の敵へ再誘導できる。
        const auto cursor = Game::GetInstance()->GetCamera()->GetAimCursor();
        BulletHoming::Search<Vector3> search(m_velocity);
        for (EnemyTarget* enemy : enemies) {
            if (!enemy || enemy->IsDead() || !enemy->GetActive()) {
                continue;
            }
            const Collision::Sphere sphere = enemy->GetCollisionSphere();
            float cursorDistanceSquared = 0.0f;
            if (!cursor.TryGetBulletScore(sphere.center, m_homingRank, cursorDistanceSquared)) {
                continue;
            }
            search.Consider(sphere.center - m_position, cursorDistanceSquared);
        }
        if (search.Found()) {
            m_velocity = BulletHoming::Turn(m_velocity, search.Offset(), m_homingRank, deltaSeconds);
        }

        // 高さ方向の誘導にもモデルの先端を追従させる。
        const float horizontal = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
        SetRotation(Vector3(DirectX::XM_PIDIV2 + std::atan2(-m_velocity.y, horizontal),
                            std::atan2(m_velocity.x, m_velocity.z), 0.0f));
    }

    // ----- 3. 移動 -----
    // 誘導後の速度で動かす。誘導先が見つからない場合は、直前の向きでそのまま進む。
    m_position += m_velocity * deltaSeconds; // 距離/秒 × 経過秒数。
    GameTime::CountDown(m_remainingLifeSeconds, deltaSeconds);

    // ----- 4. 移動した区間全体で衝突を調べる -----
    // 高速な弾は敵を1フレームで通り越すため、移動先の点だけでは判定しない。
    const Collision::Segment movement{previousPosition, m_position};

    // 弾の半径を含め、今フレームの軌道が地面に最初に触れる位置を求める。
    float groundHitFraction = 2.0f; // 1より大きい値は「このフレームでは当たらない」。
    if (const Ground* ground = Ground::Current()) {
        groundHitFraction = ground->FirstHit(previousPosition, m_position, m_colRadius).value_or(2.0f);
    }

    // 複数の敵が重なっていても、弾の軌道上で最初に当たる敵を選ぶ。
    EnemyTarget* firstEnemy = nullptr;
    float enemyHitFraction = 2.0f;
    // 共通型で取得し、飛行ドローンにもスライムと同じ線分判定を適用する。
    for (EnemyTarget* enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead() || !enemy->GetActive()) {
            continue;
        }
        float fraction = 0.0f;
        if (Collision::SegmentSphereHitFraction(movement, enemy->GetCollisionSphere(), m_colRadius,
                                                fraction) &&
            fraction < enemyHitFraction) {
            firstEnemy = enemy;
            enemyHitFraction = fraction;
        }
    }

    // 軌跡も着弾点で止め、高速な弾の残光が敵や地面を貫通して見えるのを防ぐ。
    const float travelFraction = (std::min)({1.0f, enemyHitFraction, groundHitFraction});
    const float travelSeconds = deltaSeconds * travelFraction;
    m_position = Vector3::Lerp(previousPosition, movement.end, travelFraction);
    const auto particleManagers = Game::GetInstance()->GetObjects<ParticleManager>();
    const auto leaveRibbon = [&](const Vector3& end, float timeIntoStep) {
        if (!particleManagers.empty() && end != m_trailStart) {
            CParticle* ribbon = particleManagers.front()->Spawn();
            // 粒子プールはこの直後に同じステップの時間を引くので、生成までの時間を戻す。
            if (!SetRibbon(*ribbon, m_trailStart, end, TrailLifetimeSeconds + timeIntoStep)) {
                ribbon->SetActive(false);
            }
        }
        m_trailStart = end;
    };
    m_trailElapsedSeconds += travelSeconds;
    while (m_trailElapsedSeconds >= TrailIntervalSeconds) {
        m_trailElapsedSeconds -= TrailIntervalSeconds;
        const float sampleTime = travelSeconds - m_trailElapsedSeconds;
        leaveRibbon(Vector3::Lerp(previousPosition, m_position, sampleTime / travelSeconds), sampleTime);
    }
    if (enemyHitFraction <= 1.0f || groundHitFraction <= 1.0f || m_remainingLifeSeconds <= 0.0f) {
        leaveRibbon(m_position, travelSeconds);
    }

    // ----- 5. 最初に接触した相手にだけ命中させる -----
    // 同じフレームで敵と地面の両方を通っても、手前にある方だけに命中させる。
    if (firstEnemy != nullptr && enemyHitFraction <= groundHitFraction) {
        firstEnemy->OnDirectionalHit(m_damage, m_velocity);
        if (!firstEnemy->IsDead()) {
            GameSettings::PlayImpactSound();
            if (!particleManagers.empty()) {
                CEmitter::EmitImpact(*particleManagers.front(), m_position, m_velocity);
            }
        }
        SetActive(false);
        return;
    }
    if (groundHitFraction <= 1.0f || m_remainingLifeSeconds <= 0.0f) {
        SetActive(false);
        return;
    }
}

void CBullet::Draw(Camera* cam) {
    if (m_sharedModel) {
        m_sharedModel->Draw(cam, m_position, m_rotation, ModelScale);
    }
}

void CBullet::DrawTrailTip(Camera* cam) const {
    // 次のサンプルまでの隙間も弾までつなぐ。高FPSやヒットストップ中にプールを消費しない。
    if (m_trailElapsedSeconds > 0.0f) {
        CParticle head;
        if (SetRibbon(head, m_trailStart, m_position, TrailLifetimeSeconds)) {
            head.Draw(cam);
        }
    }
}

void CBullet::Uninit() {
    m_sharedModel.reset();
}

//--------------------
//弾を発射する
//--------------------
void CBullet::Shoot(Vector3 muzzlePosition, Vector3 direction, int homingRank) {
    // プール再利用時に、以前の弾の誘導レベルを引き継がない。
    m_homingRank = std::clamp(homingRank, 0, BulletHoming::MaxRank);

    m_position = muzzlePosition;
    m_velocity = direction * LaunchSpeed;
    // モデルの長手軸を水平に倒し、発射方向へ回す。
    const float yaw = atan2f(direction.x, direction.z);
    SetRotation(Vector3(DirectX::XM_PIDIV2, yaw, 0.0f));

    m_remainingLifeSeconds = LifetimeSeconds;
    m_trailElapsedSeconds = 0.0f;
    m_trailStart = muzzlePosition;
}
