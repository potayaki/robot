#include "CMissile.h"
#include"Game.h"
CMissile::CMissile() {
    m_body = nullptr;

}

CMissile::~CMissile() {
    Uninit();

}

void CMissile::Init() {
    m_body = new TestCube;

    m_body->Init();

    m_body->SetScale(0.5f, 0.5f, 0.5f);

}

void CMissile::Update() {
    if (!m_bezier.IsActive()) {//ベジエ曲線がアクティブでない場合、ミサイルは更新されない
        Game::GetInstance()->DeleteObject(this);
        return;
    }

    float tick = 1.0f / 60.0f; //フレームレートに基づく時間
    m_bezier.Update(tick);

    //ミサイルの位置をベジエ曲線に沿って更新
    m_Position = m_bezier.GetCulvePosition(m_bezier.GetTime());

    m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z);


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
