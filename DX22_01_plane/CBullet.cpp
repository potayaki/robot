#include "CBullet.h"
#include"Game.h"
#include"CEnemy.h"
#include"Collision.h"
CBullet::CBullet() {

}

CBullet::~CBullet() {
    // Uninit();   
}

void CBullet::Init() {
    m_model = new TestModel();
    m_model->Init();
    m_model->Load("assets/model/bullet/Bullett.fbx", "assets/model/bullet");//弾のモデルを読み込む
    m_life = 60*2; //弾の寿命（フレーム数、ここでは2秒間）を設定
    m_model->SetScale(0.8f, 1.8f, 0.8f); // 弾のサイズ
    //float angleX = DirectX::XMConvertToRadians(90.0f); // 90度をラジアンに変換
    //m_model->SetRotation(DirectX::SimpleMath::Vector3( angleX,0.0f,0.0f)); // 弾の回転

}

void CBullet::Update() {


    if (m_life <= 0) {
        // 寿命が尽きたらオブジェクトを削除するなどの処理を行う
        Game::GetInstance()->DeleteObject(this);
        return;
    }

    DirectX::SimpleMath::Vector3 OldPosition = m_Position;

    m_Position += m_velocity; // 位置を更新
    m_life--; // 寿命を減らす
    m_model->SetPositin(m_Position); // モデルの位置を更新
    m_model->SetRotation(m_Rotation);

    Collision::Segment bulletsegment;
    bulletsegment.start = OldPosition;
    bulletsegment.end = m_Position;

    // 敵との当たり判定
    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

    for (auto enemy : enemies) {

        DirectX::SimpleMath::Vector3 diff = enemy->GetPosition() - m_Position;
        float distance = diff.Length();// 敵との距離を計算

        float CheckRange = 100.0f; // 当たり判定の範囲（例: 100.0f）

        if (distance < (CheckRange * CheckRange)) {// 敵が近くにいる場合のみ当たり判定を行う


            // 「Distance」で、弾と敵の距離を測る
            float dist = Collision::DistancePointToSegment(enemy->GetPosition(), bulletsegment);

            //TODO: モデルの大きさに応じて hitRange を調整する
                float hitRange = 20.0f;

                // もし距離が hitRange より近ければ「ぶつかった！」と判定
                if (dist < hitRange) {
                    // 敵に「当たったよ！」と伝える
                    enemy->OnHit(damage);

                    // 弾自身も役目を終えて消える
                    Game::GetInstance()->DeleteObject(this);
                    return; // これ以上他の敵と判定しないように、Updateを終了する

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

void CBullet::Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir) {
    //DirectX::SimpleMath::Vector3 offset(0.0f, 5.0f, 0.0f); // プレイヤーからのオフセット（弾の初期位置をプレイヤーの少し上に設定）
    DirectX::SimpleMath::Vector3 offset(0.0f, 0.0f, 0.0f); // プレイヤーからのオフセット（弾の初期位置をプレイヤーの少し上に設定）
    m_Position = player + offset; // 初期位置をセット
    m_velocity = dir * Speed; // 発射方向に速度を設定速度を掛け合わせる

    float yaw = atan2f(dir.x, dir.z);

    SetRotation(Vector3(DirectX::XMConvertToRadians(90.0f), yaw, 0.0f));


}
