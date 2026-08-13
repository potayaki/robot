#include "CBulletManager.h"

CBulletManager::CBulletManager() {
}

CBulletManager::~CBulletManager() {
    Uninit();
}

void CBulletManager::Init() {
    for (int i = 0; i < MaxBullets; i++) {
                CBullet* bullet = new CBullet();
                bullet->Init();
                bullet->SetActive(false); // 初期状態では非アクティブにする
                m_bullets.push_back(bullet);
    }
}

void CBulletManager::Update() {
    for (auto &bullet : m_bullets) {
        if (bullet->GetActive()) {
            bullet->Update();
        }
    }
}

void CBulletManager::Draw(Camera* cam) {
    for (auto& bullet : m_bullets) {
        if (bullet->GetActive()) {
            bullet->Draw(cam);
        }
    }
}

void CBulletManager::Uninit() {
    // プールしておいた弾を全て破棄する
    for (auto &bullet : m_bullets) {
        if (bullet != nullptr) {
            bullet->Uninit();
            delete bullet;
        }
    }
    m_bullets.clear();
}

void CBulletManager::ShootBullet(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& direction) {
    for (auto& bullet : m_bullets) {
        if (!bullet->GetActive()) {
            bullet->SetActive(true);
            bullet->Shoot(position, direction);
            break; // 1つの弾を発射したらループを抜ける
        }

    }
}
