/// @file ExplosionManager.cpp
/// @brief 再利用可能な爆発エフェクトの管理。

#include "ExplosionManager.h"
#ifdef _DEBUG
#include <algorithm>
#include <utility>
#endif

void ExplosionManager::Init() {
    billboard::LoadTextures("explosion", "assets/texture/PNG/Explosion/Explosion", ".png", 1);

    for (int i = 0; i < POOL_SIZE; i++) {
        m_pool[i] = new billboard();
        m_pool[i]->SetTextureGroup("explosion"); //：爆発グループに属することを教える
        m_pool[i]->Init();
        m_pool[i]->SetAnim(1.0f / 60.0f, false); // 旧60fps時の1コマ分の表示時間を維持。
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

void ExplosionManager::CreateExplosion(DirectX::SimpleMath::Vector3 pos, float sizeMultiplier) {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (!m_active[i]) {
            // 空いている爆発を見つけたら、位置をセットして再生
            m_pool[i]->SetScale(BaseDisplaySize * sizeMultiplier);
            pos.y += 15.0f; // 爆発の位置を少し上にずらす
            m_pool[i]->Play(pos);
            m_active[i] = true;
            break; // 1個出したらループ終了
        }
    }
}

#ifdef _DEBUG
void ExplosionManager::RecordDebugMissileBlast(MissileBlastDebugSample sample) {
    if (m_debugMissileBlasts.size() == MaxDebugMissileBlasts) {
        if (m_debugSelectedBlastId == m_debugMissileBlasts.front().id) {
            m_debugSelectedBlastId = 0;
        }
        m_debugMissileBlasts.pop_front();
    }
    sample.id = m_nextDebugBlastId++;
    m_debugMissileBlasts.push_back(std::move(sample));
}

const MissileBlastDebugSample* ExplosionManager::GetDisplayedDebugMissileBlast() const {
    if (m_debugMissileBlasts.empty()) {
        return nullptr;
    }
    if (m_debugSelectedBlastId != 0) {
        const auto selected =
            std::find_if(m_debugMissileBlasts.begin(), m_debugMissileBlasts.end(),
                         [this](const auto& blast) { return blast.id == m_debugSelectedBlastId; });
        if (selected != m_debugMissileBlasts.end()) {
            return &*selected;
        }
    }
    return &m_debugMissileBlasts.back();
}

void ExplosionManager::ClearDebugMissileBlasts() {
    m_debugMissileBlasts.clear();
    m_debugSelectedBlastId = 0;
}
#endif
