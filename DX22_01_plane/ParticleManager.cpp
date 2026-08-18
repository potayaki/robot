#include "ParticleManager.h"

ParticleManager::ParticleManager() {
}

ParticleManager::~ParticleManager() {
    Uninit();
}

void ParticleManager::Init() {
    for (int i = 0; i < MaxParticles; i++) {
        CParticle* p = new CParticle();
        p->Init();
        p->SetActive(false);
        m_particles.push_back(p);
    }
}

void ParticleManager::Update() {
    for (auto& a:m_particles) {
        if (a->GetActive()) {
            a->Update();
        }   
    }
}

void ParticleManager::Draw(Camera* cam) {
    for (auto& a : m_particles) {
        if (a->GetActive()) {
            a->Draw(cam);
        }
    }
}

void ParticleManager::Uninit() {
    for (auto& p : m_particles) {
        if (p != nullptr) {
            p->Uninit();
            delete p;
        }
    }
    m_particles.clear();
}

CParticle* ParticleManager::Spawn() {
    for (auto& p : m_particles) {
        if (!p->GetActive()) {
            p->SetActive(true);
            return p; // 空いているパーティクルを返す
        }
    }
    return nullptr; //すべて使用中の場合は出さない
}
