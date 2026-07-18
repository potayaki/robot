#include "CBullet.h"
#include"Game.h"
#include"CEnemy.h"

CBullet::CBullet() {

}

CBullet::~CBullet() {
   // Uninit();   
}

void CBullet::Init() {
        m_model = new TestModel();
        m_model->Init();
        m_model->Load("assets/model/bullet/Bullett.fbx","assets/model/bullet");//弾のモデルを読み込む
    m_life = 60*2; //弾の寿命（フレーム数、ここでは2秒間）を設定
    float angleX = DirectX::XMConvertToRadians(90.0f); // 90度をラジアンに変換
    m_model->SetScale(0.8f, 1.8f, 0.8f); // 弾のサイズ
    m_model->SetRotation(DirectX::SimpleMath::Vector3( angleX,0.0f,0.0f)); // 弾の回転

}

void CBullet::Update() {
   
   
    if (m_life <= 0) {
        // 寿命が尽きたらオブジェクトを削除するなどの処理を行う
        Game::GetInstance()->DeleteObject(this);
        return;
    }

    m_Position += m_velocity; // 位置を更新
    m_life--; // 寿命を減らす
    m_model->SetPositin(m_Position.x, m_Position.y, m_Position.z); // モデルの位置を更新

    // 敵との当たり判定
    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

    for (auto enemy : enemies) {
        // 「Distance」で、弾と敵の距離を測る
        float dist = DirectX::SimpleMath::Vector3::Distance(m_Position, enemy->GetPosition());

        // 当たり判定の広さ（弾の半径 + 敵の半径くらいにするのが目安）
        float hitRange = 20.0f;

        // もし距離が hitRange より近ければ「ぶつかった！」と判定
        if (dist < hitRange) {
            // 敵に「当たったよ！」と伝える
            enemy->OnHit(damage);

            // 弾自身も役目を終えて消える
            Game::GetInstance()->DeleteObject(this);

            // これ以上他の敵と判定しないように、Updateを終了する
            return;
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
    DirectX::SimpleMath::Vector3 offset(0.0f, 5.0f, 0.0f); // プレイヤーからのオフセット（弾の初期位置をプレイヤーの少し上に設定）
        m_Position = player + offset; // 初期位置をセット
    m_velocity = dir * 0.5f; // 発射方向に速度を設定（0.5fは速度の倍率）

}
