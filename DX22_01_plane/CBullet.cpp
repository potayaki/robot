#include "CBullet.h"
#include"Game.h"
#include"CEnemy.h"
#include"Collision.h"
#include"CParticle.h"
#include"Ground.h"
#include"PoolManager.h"



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
    //地面との当たり判定(Y座標より下なら消す)
    //--------------------
    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) {
        Ground* plane = grounds[0]; // 最初のGroundオブジェクトを取得
        float planeY = plane->GetPosition().y; // GroundオブジェクトのY座標を取得
        if (m_Position.y < planeY) {
            SetActive(false); // 弾を非アクティブにする
            return;
        }
    }

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

    //--------------------
    // 敵との当たり判定
   //--------------------
    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

    for (auto enemy : enemies) {

        //プレイヤーに近い敵だけ当たり判定を行うようにする（処理軽量化のため）
        DirectX::SimpleMath::Vector3 diff = enemy->GetPosition() - m_Position;
        float distance = diff.LengthSquared();

        float CheckRange = 100.0f; // 当たり判定の範囲（例: 100.0f）

        if (distance < (CheckRange * CheckRange)) {// 敵が近くにいる場合のみ当たり判定を行う


            // 「Distance」で、弾と敵の距離を測る
            float dist = Collision::DistancePointToSegment(enemy->GetPosition(), bulletsegment);

            //TODO: モデルの大きさに応じて hitRange を調整する
            float hitRange = m_colRadius + enemy->GetCollisionSphere().radius;

            // もし距離が hitRange より近ければ「ぶつかった！」と判定
            if (dist < hitRange) {
                // 敵に「当たったよ！」と伝える
                enemy->OnHit(damage);

                // 弾自身も役目を終えて消える
                SetActive(false); // 弾を非アクティブにする
                return;
            }
        }
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
