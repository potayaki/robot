#include "CBullet.h"
#include"Game.h"
#include"CEnemy.h"
#include"Collision.h"
#include"CParticle.h"
#include"Ground.h"
#include"PoolManager.h"
#include<cmath>

namespace {
// 弾の1フレーム分の軌道と敵の球が最初に接する位置を、0～1の割合で求める。
bool SegmentSphereHitFraction(const Collision::Segment& segment,
    const Collision::Sphere& sphere, float bulletRadius, float& hitFraction) {
    const Vector3 step = segment.end - segment.start;
    const float stepLengthSq = step.LengthSquared();
    if (stepLengthSq <= 0.000001f) return false;

    const Vector3 fromCenter = segment.start - sphere.center;
    const float combinedRadius = sphere.radius + bulletRadius;
    const float c = fromCenter.LengthSquared() - combinedRadius * combinedRadius;
    if (c <= 0.0f) {
        hitFraction = 0.0f; // すでに球の内側から始まっている場合。
        return true;
    }

    const float b = fromCenter.Dot(step);
    const float discriminant = b * b - stepLengthSq * c;
    if (discriminant < 0.0f) return false;

    hitFraction = (-b - std::sqrt(discriminant)) / stepLengthSq;
    return hitFraction >= 0.0f && hitFraction <= 1.0f;
}
}


CBullet::CBullet() {

}

CBullet::~CBullet() {
    // Uninit();   プール管理なので呼ばない
}

void CBullet::Init() {
    m_model = new TestModel();
    m_model->Init();
    //弾のモデルを読み込む
    m_model->Load("assets/model/bullet/Bullett.fbx", "assets/model/bullet");//弾のモデルを読み込む
    m_model->SetScale(0.8f, 4.0f, 0.8f); // 弾のサイズ

}

void CBullet::Update() {
    // 寿命が尽きたらオブジェクトを削除するなどの処理を行う
    if (m_life <= 0) {
        SetActive(false);
        return;
    }
    //パーティクルの生成で1フレーム前の弾の位置を取得するために保存しておく
    DirectX::SimpleMath::Vector3 OldPosition = m_Position;

    m_Position += m_velocity; // 位置を更新
    m_life--; // 寿命を減らす

    // モデルの位置を更新
    m_model->SetPosition(m_Position);
    m_model->SetRotation(m_Rotation);

    //--------------------
    //パーティクル(弾の軌道)を生成
    //--------------------
    std::vector<ParticleManager*> pManagers = Game::GetInstance()->GetObjects<ParticleManager>();
    if (!pManagers.empty()) {
        CParticle* trail = pManagers[0]->Spawn();
        if (trail != nullptr) { // 取得できた時だけ設定
            trail->SetType(Spark);
            trail->SetPosition(OldPosition);
            trail->SetVelocity(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
            trail->SetLife(20);
            trail->SetScale(DirectX::SimpleMath::Vector3(2.5f, 2.5f, 2.5f));
            trail->SetColor(DirectX::SimpleMath::Color(1.0f, 0.0f, 0.0f, 1.0f));
        }

    }

    //すり抜け防止のために、弾の前回位置と現在位置を結ぶ線分を作る
    Collision::Segment bulletsegment;
    bulletsegment.start = OldPosition;
    bulletsegment.end = m_Position;

    // 弾の半径を含め、今フレームの軌道が地面に最初に触れる位置を求める。
    float groundHitFraction = 2.0f; // 1より大きい値は「このフレームでは当たらない」。
    const std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty() && grounds[0] != nullptr) {
        const float groundCollisionY = grounds[0]->GetPosition().y + m_colRadius;
        if (OldPosition.y <= groundCollisionY) {
            groundHitFraction = 0.0f;
        }
        else if (m_Position.y <= groundCollisionY) {
            groundHitFraction = (groundCollisionY - OldPosition.y) /
                (m_Position.y - OldPosition.y);
        }
    }

    // 複数の敵が重なっていても、弾の軌道上で最初に当たる敵を選ぶ。
    CEnemy* firstEnemy = nullptr;
    float enemyHitFraction = 2.0f;
    for (CEnemy* enemy : Game::GetInstance()->GetObjects<CEnemy>()) {
        if (enemy == nullptr || enemy->IsDead()) continue;
        float fraction = 0.0f;
        if (SegmentSphereHitFraction(bulletsegment, enemy->GetCollisionSphere(),
                m_colRadius, fraction) && fraction < enemyHitFraction) {
            firstEnemy = enemy;
            enemyHitFraction = fraction;
        }
    }

    // 同じフレームで敵と地面の両方を通っても、手前にある方だけに命中させる。
    if (firstEnemy != nullptr && enemyHitFraction <= groundHitFraction) {
        firstEnemy->OnHit(damage);
        SetActive(false);
        return;
    }
    if (groundHitFraction <= 1.0f) {
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
void CBullet::Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir) {
   
    DirectX::SimpleMath::Vector3 offset(0.0f, 0.0f, 0.0f); //弾の初期位置のオフセットを設定(モデル入れ替えの際に微調整)
    m_Position = player + offset; // 初期位置をセット
    m_velocity = dir * Speed; // 発射方向に速度を設定速度を掛け合わせる

    //弾の向きを発射方向に合わせるために、Y軸の回転角度を計算する
    float yaw = atan2f(dir.x, dir.z);

    // 弾の回転を設定（X軸を90度回転させ、Y軸は発射方向に合わせる）
    SetRotation(Vector3(DirectX::XMConvertToRadians(90.0f), yaw, 0.0f));

    m_life = 60 * 2; //弾の寿命（フレーム数、ここでは2秒間）を設定
}
