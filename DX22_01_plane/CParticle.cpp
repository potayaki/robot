#include "CParticle.h"

CParticle::CParticle() {
    m_body = nullptr;
    m_life = 60.0f; // デフォルトは60フレーム
}

CParticle::~CParticle() {
    Uninit();
}

void CParticle::Init() {
    m_body = new TestCube;
    m_body->Init();
    m_body->SetScale(0.5f, 0.5f, 0.5f); // 破片なので少し小さくする
}

void CParticle::Update() {
    // 速度の分だけ移動
    m_Position += m_velocity;

    // 重力をかけて少しずつ落ちるようにする（お好みで！）
    m_velocity.y -= 0.02f;

    // クルクル回転させると破片っぽさが出る
    m_Rotation.x += 0.1f;
    m_Rotation.y += 0.1f;

    // 姿勢をボディに反映（※元のコードのスペル「SetPositin」に合わせています）
    m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z);
    m_body->SetRotation(m_Rotation);

    // 寿命を減らして、0になったら自分を消す
    m_life -= 1.0f;
    if (m_life <= 0.0f) {
        Destroy(); // 先ほど追加した削除フラグ関数
    }
}

void CParticle::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void CParticle::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}
