/// @file CExperienceOrbManager.cpp
/// @brief 敵の撃破位置で落下を開始し、吸収した経験値をまとめて反映する。
#include "CExperienceOrbManager.h"
#include "CPlayer.h"
#include "ExperienceOrbDrop.h"
#include "ForestTerrain.h"
#include "Game.h"
#include "GameSettings.h"
#include "Ground.h"

using namespace DirectX::SimpleMath;

void CExperienceOrbManager::Init() {
    m_renderer.Init();
}

void CExperienceOrbManager::Drop(const Vector3& position, int dropWeight) {
    const auto* ground = Ground::Current();
    const float groundY = ground ? ground->HeightAt(position.x, position.z) : ForestLayout::GroundY;
    AppendExperienceOrbDrop(m_orbs, position, groundY, dropWeight, m_random);
}

void CExperienceOrbManager::Update() {
    if (m_orbs.empty()) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty()) {
        return;
    }
    auto* player = players.front();
    if (player->IsDead() || player->GetHp() <= 0 || player->IsGameplayPaused()) {
        return;
    }
    const auto body = player->GetCollisionSphere();
    const Vector3 receiver = body.center + Vector3::Up * (body.radius * 0.4f);
    const auto* ground = Ground::Current();
    const auto heightAt = [ground](float x, float z) {
        return ground ? ground->HeightAt(x, z) : ForestLayout::GroundY;
    };
    const float seconds = GameTime::DeltaSeconds();
    int collected = 0;
    for (size_t i = 0; i < m_orbs.size();) {
        auto& orb = m_orbs[i];
        if (orb.Advance(seconds, receiver, heightAt)) {
            collected += orb.experience;
            // 描画順に意味はないので、末尾の粒を移して同じ添字から更新を続ける。
            orb = m_orbs.back();
            m_orbs.pop_back();
            continue;
        }
        // 小さな尾根を越えて機体へ向かうときも、粒を地面の上に見せる。
        if (orb.IsAttracting()) {
            constexpr float clearance = 3.0f;
            orb.position.y = (std::max)(orb.position.y, heightAt(orb.position.x, orb.position.z) + clearance);
        }
        ++i;
    }
    if (collected > 0) {
        player->GainExperience(collected);
        GameSettings::PlayExperienceSound();
    }
}

void CExperienceOrbManager::Draw(Camera* camera) {
    m_renderer.Draw(camera, m_orbs);
}

void CExperienceOrbManager::Uninit() {
    m_orbs.clear();
    m_renderer.Uninit();
}
