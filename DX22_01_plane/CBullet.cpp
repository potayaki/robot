/// @file CBullet.cpp
/// @brief 通常弾の発射、移動、誘導、移動区間での命中判定。

#include "CBullet.h"
#include "Game.h"
#include "EnemyTarget.h"
#include "Collision.h"
#include "CParticle.h"
#include "Ground.h"
#include "PoolManager.h"
#include "BulletHoming.h"
#include "AimCursor.h"
#include "CPlayer.h"
#include <cmath>

namespace {
    // 弾の1フレーム分の軌道と敵の球が最初に接する位置を、0～1の割合で求める。
    bool SegmentSphereHitFraction(const Collision::Segment& segment, const Collision::Sphere& sphere,
                                  float bulletRadius, float& hitFraction) {
        const Vector3 step = segment.end - segment.start;
        const float stepLengthSq = step.LengthSquared();
        if (stepLengthSq <= 0.000001f) {
            return false;
        }

        const Vector3 fromCenter = segment.start - sphere.center;
        const float combinedRadius = sphere.radius + bulletRadius;
        const float c = fromCenter.LengthSquared() - combinedRadius * combinedRadius;
        if (c <= 0.0f) {
            hitFraction = 0.0f; // すでに球の内側から始まっている場合。
            return true;
        }

        const float b = fromCenter.Dot(step);
        // 線分と球の交点を求める二次方程式の判別式。負なら接触しない。
        const float discriminant = b * b - stepLengthSq * c;
        if (discriminant < 0.0f) {
            return false;
        }

        // 2つの交点のうち手前を使う。0なら始点、1なら今フレームの終点。
        hitFraction = (-b - std::sqrt(discriminant)) / stepLengthSq;
        return hitFraction >= 0.0f && hitFraction <= 1.0f;
    }
} // namespace

CBullet::CBullet() {}

CBullet::~CBullet() {
    // Uninit();   プール管理なので呼ばない
}

void CBullet::Init() {
    m_model = new TestModel();
    m_model->Init();
    //弾のモデルを読み込む
    m_model->Load("assets/model/bullet/Bullett.fbx", "assets/model/bullet"); //弾のモデルを読み込む
    m_model->SetScale(m_modelScale); // 弾のサイズ
}

void CBullet::Update() {
    // ----- 1. 寿命の確認 -----
    // 弾は削除せず非アクティブにして、プールから次の発射時に再利用する。
    if (m_life <= 0) {
        SetActive(false);
        return;
    }

    // 寿命が切れる最後の更新では、残っている時間の分だけ移動する。
    const float deltaSeconds = (std::min)(GameTime::DeltaSeconds(), m_life);

    //パーティクルの生成で1フレーム前の弾の位置を取得するために保存しておく
    DirectX::SimpleMath::Vector3 OldPosition = m_position;

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

    // ----- 3. 移動とモデルの更新 -----
    // 誘導後の速度で動かす。誘導先が見つからない場合は、直前の向きでそのまま進む。
    m_position += m_velocity * deltaSeconds; // 距離/秒 × 経過秒数。
    GameTime::CountDown(m_life, deltaSeconds);

    // モデルの位置を更新
    m_model->SetPosition(m_position);
    m_model->SetRotation(m_rotation);

    //--------------------
    //パーティクル(弾の軌道)を生成
    //--------------------
    std::vector<ParticleManager*> pManagers = Game::GetInstance()->GetObjects<ParticleManager>();
    m_trailTimer += deltaSeconds;
    // 粒子の生成間隔は弾ごとの調整値を参照する。
    while (m_trailTimer >= m_trailInterval) {
        m_trailTimer -= m_trailInterval;
        if (pManagers.empty()) {
            continue;
        }
        CParticle* trail = pManagers[0]->Spawn();
        if (trail != nullptr) { // 取得できた時だけ設定
            trail->SetType(Spark);
            // 低FPSで複数生成するときも、移動した区間に等間隔で並べる。
            const float fraction = 1.0f - m_trailTimer / deltaSeconds;
            trail->SetPosition(Vector3::Lerp(OldPosition, m_position, fraction));
            trail->SetVelocity(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
            trail->SetLife(m_trailLifetime);
            trail->SetScale(DirectX::SimpleMath::Vector3(m_trailScale, m_trailScale, m_trailScale));
            trail->SetColor(DirectX::SimpleMath::Color(1.0f, 0.0f, 0.0f, 1.0f));
        }
    }

    // ----- 4. 移動した区間全体で衝突を調べる -----
    // 高速な弾は敵を1フレームで通り越すため、移動先の点だけでは判定しない。
    Collision::Segment bulletsegment;
    bulletsegment.start = OldPosition;
    bulletsegment.end = m_position;

    // 弾の半径を含め、今フレームの軌道が地面に最初に触れる位置を求める。
    float groundHitFraction = 2.0f; // 1より大きい値は「このフレームでは当たらない」。
    const std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty() && grounds[0] != nullptr) {
        const float groundCollisionY = grounds[0]->GetPosition().y + m_colRadius;
        if (OldPosition.y <= groundCollisionY) {
            groundHitFraction = 0.0f;
        } else if (m_position.y <= groundCollisionY) {
            groundHitFraction = (groundCollisionY - OldPosition.y) / (m_position.y - OldPosition.y);
        }
    }

    // 複数の敵が重なっていても、弾の軌道上で最初に当たる敵を選ぶ。
    EnemyTarget* firstEnemy = nullptr;
    float enemyHitFraction = 2.0f;
    // 共通型で取得し、飛行ドローンにもスライムと同じ線分判定を適用する。
    for (EnemyTarget* enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead()) {
            continue;
        }
        float fraction = 0.0f;
        if (SegmentSphereHitFraction(bulletsegment, enemy->GetCollisionSphere(), m_colRadius, fraction) &&
            fraction < enemyHitFraction) {
            firstEnemy = enemy;
            enemyHitFraction = fraction;
        }
    }

    // ----- 5. 最初に接触した相手にだけ命中させる -----
    // 同じフレームで敵と地面の両方を通っても、手前にある方だけに命中させる。
    if (firstEnemy != nullptr && enemyHitFraction <= groundHitFraction) {
        firstEnemy->OnHit(m_damage);
        SetActive(false);
        return;
    }
    if (groundHitFraction <= 1.0f || m_life <= 0.0f) {
        SetActive(false);
        return;
    }
}

void CBullet::Draw(Camera* cam) {
    if (m_model) {
        m_model->Draw(cam);
    }
}

void CBullet::Uninit() {
    if (m_model) {
        m_model->Uninit();
        delete m_model;
        m_model = nullptr;
    }
}

//--------------------
//弾を発射する
//--------------------
void CBullet::Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir, int homingRank) {
    // プール再利用時に、以前の弾の誘導レベルを引き継がない。
    m_homingRank = std::clamp(homingRank, 0, BulletHoming::MaxRank);

    DirectX::SimpleMath::Vector3 offset(0.0f, 0.0f,
                                        0.0f); //弾の初期位置のオフセットを設定(モデル入れ替えの際に微調整)
    m_position = player + offset; // 初期位置をセット
    m_velocity = dir * m_speed; // 発射方向に速度を設定速度を掛け合わせる

    //弾の向きを発射方向に合わせるために、Y軸の回転角度を計算する
    float yaw = atan2f(dir.x, dir.z);

    // 弾の回転を設定（X軸を90度回転させ、Y軸は発射方向に合わせる）
    SetRotation(Vector3(DirectX::XM_PIDIV2, yaw, 0.0f));

    m_life = m_lifetimeSeconds; // 残り秒数から実測の経過時間を引くので、FPSによらず2秒で消える。
    m_trailTimer = 0.0f;
}
