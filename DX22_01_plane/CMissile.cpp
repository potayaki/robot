#include "CMissile.h"
#include"Game.h"
#include<cmath>
#include"CEnemy.h"
CMissile::CMissile() {
  
    m_body = nullptr;
}

CMissile::~CMissile() {
    Uninit();

}

void CMissile::Init() {
    m_body = new TestModel;

    m_body->Init();

    m_body->Load("assets/model/bullet/Bullett.fbx", "assets/model/bullet");

    m_body->SetScale(2.5f, 2.0f, 2.5f);

}

void CMissile::Update() {
    if (!m_bezier.IsActive()) {//ベジエ曲線がアクティブでない場合、ミサイルは更新されない
        Game::GetInstance()->DeleteObject(this);
        return;
    }

    float tick = 1.0f/ 60.0f;//60FPSで更新するための時間を計算
    m_bezier.Update(tick);

    //ミサイルの位置をベジエ曲線に沿って更新
    m_Position = m_bezier.GetCulvePosition(m_bezier.GetTime());

    m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z);

    //角度を計算してミサイルの向きを更新
    //現在
    float currentTime = m_bezier.GetTime();
    DirectX::SimpleMath::Vector3 currentPos = m_bezier.GetCulvePosition(currentTime);

    //少し先の未来
    float futureTime = currentTime + 0.01f; // 未来の時間を少し進める
    if (futureTime > 1.0f) {
        futureTime = 1.0f; // 未来の時間が1.0を超えないようにする
    }
    DirectX::SimpleMath::Vector3 futurePos = m_bezier.GetCulvePosition(futureTime);

    //現在の位置と未来の位置から方向ベクトルを計算
    DirectX::SimpleMath::Vector3 direction;
    direction = futurePos - currentPos;
    direction.Normalize();

    float Yaw = atan2f(direction.x, direction.z); // Y軸周りの回転角度

    float XZLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
    float Pitch = atan2f(-direction.y, XZLength); // X軸周りの回転角度

    m_body->SetRotation(DirectX::SimpleMath::Vector3(Pitch + DirectX::XM_PIDIV2, Yaw, 0.0f)); // ミサイルの回転を更新

    //TODO : 今は数が少ないからいいけど多くなったら重くなるから今後カメラの中だけとかでする
    //敵を全部取得
    std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

    for (auto &enemy:enemies) {

                // 「Distance」で、ミサイルと敵の距離を測る
        float dist = DirectX::SimpleMath::Vector3::Distance(m_Position, enemy->GetPosition());

        // 当たり判定の広さ（ミサイルの半径 + 敵の半径くらいにするのが目安）
        float hitRange = 20.0f;

        // もし距離が hitRange より近ければ「ぶつかった！」と判定
        if (dist < hitRange) {
            // 敵に「当たったよ！」と伝える
            enemy->OnHit(damage); // ダメージを与える

            // ミサイル自身も役目を終えて消える
            Game::GetInstance()->DeleteObject(this);

            // これ以上他の敵と判定しないように、Updateを終了する
            return;
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

void CMissile::Shoot(Object& shooter, Object& target) {

    m_bezier.Create(shooter,target); //ベジエ曲線を作成

}
