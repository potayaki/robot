/// @brief ステージ上の経験値の粒を保持し、機体に届いた分だけ経験値を渡す。
#pragma once
#include "ExperienceOrb.h"
#include "ExperienceOrbRenderer.h"
#include "Object.h"
#include <random>
#include <vector>

class CExperienceOrbManager : public Object {
public:
    void Init() override;
    void Update() override;
    void Draw(Camera* camera) override;
    void Uninit() override;
    /// 撃破した敵のワールド座標から8〜10粒を落とす。重みが0以下なら報酬はない。
    void Drop(const DirectX::SimpleMath::Vector3& position, int dropWeight);

private:
    /// 敵や短命の演出パーティクルと独立して、回収されるまでステージに残す。
    std::vector<ExperienceOrb> m_orbs;
    /// 粒の抽選と飛び方を、敵の出現・装備抽選とは独立に決める乱数。
    std::mt19937 m_random{std::random_device{}()};
    /// 全ての粒の描画資源を共有する。
    ExperienceOrbRenderer m_renderer;
};
