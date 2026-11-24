/// @file CMissile.cpp
/// @brief ミサイルの軌道追従、命中、爆発と画面揺れ。

#include "CMissile.h"
#include "Game.h"
#include "GameSettings.h"
#include <algorithm>
#include <cmath>
#include "EnemyTarget.h"
#include "CPlayer.h"
#include "Collision.h"
#include "ExplosionManager.h"

void CMissile::Init() {
    static std::weak_ptr<TestModel> cachedMissileModel;
    m_sharedModel = TestModel::AcquireShared(cachedMissileModel, "assets/model/rocket/cruisemissile.fbx",
                                             "assets/model/rocket");
}

void CMissile::Update() {
    if (!m_bezier.IsActive()) {
        SetActive(false);
        return;
    }

    Game* game = Game::GetInstance();
    // 同じ敵リストを、追尾先の生存確認と移動後の衝突判定に使う。
    const auto enemies = game->GetObjects<EnemyTarget>();
    if (m_target != nullptr) {
        bool targetIsAlive = false;
        // 消えた敵のポインタを参照しないよう、現在存在する共通の敵リストで確認する。
        for (EnemyTarget* enemy : enemies) {
            if (enemy == m_target && enemy->GetActive() && !enemy->IsDead()) {
                targetIsAlive = true;
                break;
            }
        }
        if (targetIsAlive) {
            m_bezier.UpdateTargetPosition(m_target->GetPosition());
        } else {
            m_target = nullptr;
        }
    }

    const DirectX::SimpleMath::Vector3 previousPosition = m_position;
    m_bezier.Update(GameTime::DeltaSeconds());
    const float progress = m_bezier.GetTime();
    m_position = m_bezier.GetCulvePosition(progress);
    const Collision::Segment movement{previousPosition, m_position};

    // 軌道上の少し先の位置へ機首を向ける。進行率1が終点。
    const float lookAheadProgress = (std::min)(progress + DirectionLookAheadFraction, 1.0f);
    const DirectX::SimpleMath::Vector3 lookAheadPosition = m_bezier.GetCulvePosition(lookAheadProgress);
    DirectX::SimpleMath::Vector3 direction = lookAheadPosition - m_position;
    // 軌道の終点では未来位置と一致するため、直前の移動方向を使う。
    if (direction.LengthSquared() < 0.000001f) {
        direction = m_position - previousPosition;
    }
    if (direction.LengthSquared() > 0.000001f) {
        direction.Normalize();
    }

    const float yaw = atan2f(direction.x, direction.z);
    const float horizontalLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
    const float pitch = atan2f(-direction.y, horizontalLength);
    SetRotation(DirectX::SimpleMath::Vector3(pitch + DirectX::XM_PIDIV2, yaw, 0.0f));

    // 追尾先が先に倒れても最後に狙った場所で炸裂する。
    float firstFraction = 2.0f;
    for (EnemyTarget* enemy : enemies) {
        if (enemy == nullptr || !enemy->GetActive() || enemy->IsDead()) {
            continue;
        }
        float fraction = 0.0f;
        if (Collision::SegmentSphereHitFraction(movement, enemy->GetCollisionSphere(), m_colRadius,
                                                fraction)) {
            firstFraction = (std::min)(firstFraction, fraction);
        }
    }
    if (firstFraction <= 1.0f) {
        m_position = DirectX::SimpleMath::Vector3::Lerp(movement.start, movement.end, firstFraction);
    } else if (m_bezier.IsActive()) {
        return;
    }

    // 直撃も爆風に含め、重なった敵は1体につき一度だけ撃破処理へ渡す。
    SetActive(false);
    GameSettings::PlayExplosionSound();
    for (EnemyTarget* enemy : enemies) {
        if (enemy && enemy->GetActive() && !enemy->IsDead()) {
            int damage = BlastDamage(m_damage, m_position, enemy->GetCollisionSphere());
            if (damage > 0) {
                enemy->OnDirectionalHit(damage, enemy->GetPosition() - m_position);
            }
        }
    }
    const auto explosionManagers = game->GetObjects<ExplosionManager>();
    if (!explosionManagers.empty() && explosionManagers[0] != nullptr) {
        explosionManagers[0]->CreateExplosion(m_position);
    }

    // プレイヤーから遠い爆発ほど画面揺れを弱める。
    const auto players = game->GetObjects<CPlayer>();
    if (!players.empty() && players[0] != nullptr) {
        const float playerDistance = (players[0]->GetPosition() - m_position).Length();
        if (playerDistance < MaxShakeDistance) {
            const float shakePower = MaxShakePower * (1.0f - playerDistance / MaxShakeDistance);
            game->GetCamera()->SetShake(ShakeSeconds, shakePower);
        }
    }

    const auto billboardPools = game->GetObjects<ExplosinManager>();
    if (!billboardPools.empty()) {
        if (billboard* explosion = billboardPools[0]->Spawn()) {
            explosion->SetPosition(m_position.x, m_position.y, m_position.z);
            explosion->SetScale(ExplosionScale, ExplosionScale, ExplosionScale);
            explosion->SetAnim(ExplosionFrameSeconds, false);
        }
    }
}

void CMissile::Draw(Camera* cam) {
    if (m_sharedModel) {
        m_sharedModel->Draw(cam, m_position, m_rotation, ModelScale);
    }
}

void CMissile::Uninit() {
    m_sharedModel.reset();
}

void CMissile::Shoot(Object& shooter, Object& target, float angleOffsetDegrees) {
    // プールから再利用したときも、前回の命中状態を引き継がない。
    SetActive(true);
    // 前回の着弾地点と今回の軌道をつながず、発射位置から当たり判定を開始する。
    m_position = shooter.GetPosition();
    m_target = &target;
    m_bezier.Create(shooter, target, angleOffsetDegrees);
}
