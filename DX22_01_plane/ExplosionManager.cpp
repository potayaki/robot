#include "ExplosionManager.h"

void ExplosionManager::Init() {
    billboard::LoadTextures("explosion", "assets/texture/PNG/Explosion/Explosion",".png",1);

    for (int i = 0; i < POOL_SIZE; i++) {
        m_pool[i] = new billboard();
        m_pool[i]->SetTextureGroup("explosion"); //：爆発グループに属することを教える
        m_pool[i]->Init();
        m_pool[i]->SetAnim(0.005f, false); 
        m_pool[i]->SetSpriteSheet(4, 4, 16);
        m_active[i] = false;
    }
}

void ExplosionManager::Update() {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (m_active[i]) {
            m_pool[i]->Update(); // アクティブなものだけコマを進める

            // アニメーションが終わったら非アクティブ（空き）にする
            if (m_pool[i]->IsFinished()) {
                m_active[i] = false;
            }
        }
    }
}

void ExplosionManager::Draw(Camera* cam) {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (m_active[i]) {
            m_pool[i]->Draw(cam); // アクティブなものだけ描画
        }
    }
}

void ExplosionManager::Uninit() {
    for (int i = 0; i < POOL_SIZE; i++) {
        m_pool[i]->Uninit();
        delete m_pool[i];
    }
    billboard::ReleaseTextures("explosion");
}

void ExplosionManager::CreateExplosion(DirectX::SimpleMath::Vector3 pos) {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (!m_active[i]) {
            // 空いている爆発を見つけたら、位置をセットして再生
            m_pool[i]->SetScale(200.0f); // 爆発の大きさ
            pos.y += 15.0f; // 爆発の位置を少し上にずらす
            m_pool[i]->Play(pos);
            m_active[i] = true;
            break; // 1個出したらループ終了
        }
    }
}
