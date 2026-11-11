/// @file CMissile.cpp
/// @brief ミサイルの軌道追従、命中、爆発と画面揺れ。

#include "CMissile.h"
#include "Game.h"
#include <cmath>
#include "EnemyTarget.h"
#include "CPlayer.h"
#include "Collision.h"
#include "CParticle.h"
#include "ExplosionManager.h"

#include "billboard.h"

#include "PoolManager.h"

CMissile::CMissile() {
    m_body = nullptr;
}

CMissile::~CMissile() {
    Uninit();
}

void CMissile::Init() {
    m_body = new TestModel;

    m_body->Init();

    m_body->Load("assets/model/rocket/cruisemissile.fbx", "assets/model/rocket");

    m_body->SetScale(m_modelScale);
}

void CMissile::Update() {
    if (!m_bezier.IsActive()) { //ベジエ曲線がアクティブでない場合、ミサイルは更新されない
        SetActive(false); // ミサイルを破棄
        return;
    }

    if (m_target != nullptr) {
        bool targetIsAlive = false;
        // 消えた敵のポインタを参照しないよう、現在存在する共通の敵リストで確認する。
        for (EnemyTarget* enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy == m_target && !enemy->IsDead()) {
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

    DirectX::SimpleMath::Vector3 oldPosition = m_position;

    const float tick = GameTime::DeltaSeconds(); // 実測した秒数だけ軌道を進める。
    m_bezier.Update(tick);

    //ミサイルの位置をベジエ曲線に沿って更新
    m_position = m_bezier.GetCulvePosition(m_bezier.GetTime());

    m_body->SetPosition(m_position.x, m_position.y, m_position.z);

    Collision::Segment MissileSegment;

    MissileSegment.start = oldPosition;
    MissileSegment.end = m_position;

    //角度を計算してミサイルの向きを更新
    float currentTime = m_bezier.GetTime();
    DirectX::SimpleMath::Vector3 currentPos = m_bezier.GetCulvePosition(currentTime);

    //少し先の未来
    float futureTime = currentTime + m_directionLookAhead; // 未来の時間を少し進める
    if (futureTime > 1.0f) {
        futureTime = 1.0f; // 未来の時間が1.0を超えないようにする
    }
    DirectX::SimpleMath::Vector3 futurePos = m_bezier.GetCulvePosition(futureTime);

    //現在の位置と未来の位置から方向ベクトルを計算
    DirectX::SimpleMath::Vector3 direction;
    direction = futurePos - currentPos;
    // 軌道の終点では未来位置と一致するため、直前の移動方向を使う。
    if (direction.LengthSquared() < 0.000001f) {
        direction = m_position - oldPosition;
    }
    if (direction.LengthSquared() > 0.000001f) {
        direction.Normalize();
    }

    float Yaw = atan2f(direction.x, direction.z); // Y軸周りの回転角度

    float XZLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
    float Pitch = atan2f(-direction.y, XZLength); // X軸周りの回転角度

    m_body->SetRotation(
        DirectX::SimpleMath::Vector3(Pitch + DirectX::XM_PIDIV2, Yaw, 0.0f)); // ミサイルの回転を更新

    //TODO : 今は数が少ないからいいけど多くなったら重くなるから今後カメラの中だけとかでする
    //敵を全部取得
    // ドローンを含む全敵を、それぞれの当たり判定半径で判定する。
    std::vector<EnemyTarget*> enemies = Game::GetInstance()->GetObjects<EnemyTarget>();

    for (auto& enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead()) {
            continue;
        }

        const float checkRange = m_collisionSearchRange; // 衝突候補の検索距離。

        // 経過時間が長い場合も、終点だけでなく移動区間全体を候補判定する。
        if (Collision::DistancePointToSegment(enemy->GetPosition(), MissileSegment) < checkRange) {
            // 「Distance」で、ミサイルと敵の距離を測る
            float dist = Collision::DistancePointToSegment(enemy->GetPosition(), MissileSegment);

            // ミサイルと敵の球の半径を合算し、空中のドローンにも正しく命中させる。
            float hitRange = m_colRadius + enemy->GetCollisionSphere().radius;

            //当たったかどうかの判定
            if (dist < hitRange) {
                enemy->OnHit(m_damage);

                // 爆発エフェクトはミサイルが敵に命中したときだけ生成する。
                std::vector<ExplosionManager*> explosionManagers =
                    Game::GetInstance()->GetObjects<ExplosionManager>();
                if (!explosionManagers.empty() && explosionManagers[0] != nullptr) {
                    explosionManagers[0]->CreateExplosion(enemy->GetPosition());
                }

                // 画面揺れもミサイル命中時だけ発生させ、プレイヤーから遠い爆発ほど弱くする。
                std::vector<CPlayer*> players = Game::GetInstance()->GetObjects<CPlayer>();
                if (!players.empty() && players[0] != nullptr) {
                    float distance = (players[0]->GetPosition() - enemy->GetPosition()).Length();
                    // プレイヤーからの距離に応じて画面揺れを弱める。
                    if (distance < m_maxShakeDistance) {
                        float shakePower = m_maxShakePower * (1.0f - distance / m_maxShakeDistance);
                        Game::GetInstance()->GetCamera()->SetShake(m_shakeSeconds, shakePower);
                    }
                }

                std::vector<ParticleManager*> pManagers = Game::GetInstance()->GetObjects<ParticleManager>();

                if (!pManagers.empty()) {
                    /*
                    // パーティクルを生成
                    for (size_t i = 0; i < 20; i++) {// 20個のパーティクルを生成
                        CParticle* p = pManagers[0]->Spawn();
                        if (p != nullptr) {
                            p->SetPosition(m_Position.x, m_Position.y, m_Position.z);

                            // 飛ぶ方向をランダムにする (-1.0f ~ 1.0f の範囲)
                            float vx = (rand() % 100 / 50.0f) - 1.0f;
                            float vy = (rand() % 100 / 50.0f) - 1.0f;
                            float vz = (rand() % 100 / 50.0f) - 1.0f;

                            // 上方向にする
                            vy += 1.5f;

                            p->SetVelocity(DirectX::SimpleMath::Vector3(vx, vy, vz));

                            // 寿命もバラバラにする（0.5〜1秒）
                            p->SetLife(0.5f + (rand() % 30) / 60.0f);
                        }
                    }
                    */

                    //billboardのエフェクトを生成

                    std::vector<ExplosinManager*> managers =
                        Game::GetInstance()->GetObjects<ExplosinManager>();
                    if (!managers.empty()) {
                        billboard* b = managers[0]->Spawn();
                        if (b) {
                            b->SetPosition(m_position.x, m_position.y, m_position.z);
                            b->SetScale(m_explosionScale, m_explosionScale, m_explosionScale);
                            b->SetAnim(m_explosionFrameSeconds, false); // アニメーション速度とループ設定
                        }
                    }
                }

                // パーティクル管理が無い場合も命中は一度だけにし、爆発を連続生成しない。
                SetActive(false);
                return;
            }
        }
    }
}

void CMissile::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void CMissile::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

void CMissile::Shoot(Object& shooter, Object& target, float angleOffsetDebug) {
    // プールから再利用したときも、前回の命中状態を引き継がない。
    SetActive(true);
    // 前回の着弾地点と今回の軌道をつながず、発射位置から当たり判定を開始する。
    m_position = shooter.GetPosition();
    m_body->SetPosition(m_position);
    m_target = &target;
    m_bezier.Create(shooter, target, angleOffsetDebug); //ベジエ曲線を作成
}
