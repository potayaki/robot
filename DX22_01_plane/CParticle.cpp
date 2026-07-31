#include "CParticle.h"
#include "Game.h"

// static変数の実体を定義（必ず関数の外に書く）
TestCube* CParticle::m_body = nullptr;
int CParticle::m_instanceCount = 0;

CParticle::CParticle() {
    m_life = 60.0f;
}

CParticle::~CParticle() {
    Uninit();
}

void CParticle::Init() {
    // 「一番最初の1個目」が作られた時だけ、モデルを読み込む
    if (m_instanceCount == 0) {
        m_body = new TestCube;
        m_body->Init();
        m_body->SetScale(0.5f, 0.5f, 0.5f);
    }
    m_instanceCount++; // 生成されたらカウントを増やす
}

void CParticle::Update() {
    // 速度の分だけ移動
    m_Position += m_velocity;
    m_velocity.y -= 0.02f; // 重力

    // クルクル回転させる
    m_Rotation.x += 0.1f;
    m_Rotation.y += 0.1f;

    // 寿命を減らして、0になったら自分を消す
    m_life -= 1.0f;
    if (m_life <= 0.0f) {
        Destroy();
    }
}

void CParticle::Draw(Camera* cam) {
    if (m_body) {
        // 共有モデルを「このパーティクルの位置と角度」に移動させてから描く
        m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z);
        m_body->SetRotation(m_Rotation);
        m_body->Draw(cam);
    }
}

void CParticle::Uninit() {
   
}
