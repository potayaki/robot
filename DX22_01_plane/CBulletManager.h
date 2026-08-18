//今は使わないPoolManagerを使う
#pragma once
#include "Object.h"
#include "CBullet.h"
class CBulletManager : public Object
{
private:
    static const int MaxBullets = 30; // 最大弾数
    std::vector<CBullet*> m_bullets; // 弾のリスト
public:
    CBulletManager();
    ~CBulletManager();
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;
    // 弾を発射する関数
    void ShootBullet(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& direction);
};

