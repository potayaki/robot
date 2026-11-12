/// @file CGhostEnemy.cpp
/// @brief 支援線と強化効果を同じ対象選択で処理し、孤立時は霊弾で攻撃する。
#include "CGhostEnemy.h"
#include "Game.h"
#include "Ground.h"
#include "CPlayer.h"
#include "CEnemy.h"
#include "CBossBattle.h"
#include "imgui.h"
#include <cmath>
using namespace DirectX::SimpleMath;

GhostSupport::Settings& CGhostEnemy::SupportSettings() {
    static GhostSupport::Settings settings;
    return settings;
}

void CGhostEnemy::Init() {
    m_model = GhostModel::Acquire();
    if (!m_model) {
        Destroy();
    }
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    m_position.y = (grounds.empty() ? -5.0f : grounds[0]->GetPosition().y) + m_hoverHeight;
}

std::vector<EnemyTarget*> CGhostEnemy::GetSupportTargets() const {
    // ポインタを次フレームへ保持せず、その時点の生存者だけを参照する。
    // Ghost死亡直後の同一フレームでも、描画・与ダメージ・被ダメージすべてで支援が消える。
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (IsDead() || !GetActive() || players.empty() || players[0]->GetHp() <= 0) {
        return {};
    }
    const auto enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
    std::vector<GhostSupport::Candidate> candidates;
    for (size_t i = 0; i < enemies.size(); ++i) {
        auto enemy = enemies[i];
        candidates.push_back(
            {i, (enemy->GetPosition() - m_position).LengthSquared(),
             !enemy->IsDead() && enemy->GetActive() && dynamic_cast<CGhostEnemy*>(enemy) == nullptr});
    }
    std::vector<EnemyTarget*> result;
    for (size_t index : GhostSupport::Select(candidates, SupportSettings(), true)) {
        result.push_back(enemies[index]);
    }
    return result;
}

void CGhostEnemy::Update() {
    if (IsDead()) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        m_shots.clear();
        return;
    }
    auto player = players[0];
    const float dt = GameTime::DeltaSeconds();
    Vector3 direction = player->GetPosition() - m_position;
    direction.y = 0;
    const float distance = direction.Length();
    if (distance > 0.0001f) {
        direction /= distance;
    } else {
        direction = Vector3(0, 0, 1);
    }
    m_position += direction * GhostSupport::Travel(distance, m_moveSpeed, dt, m_nearDistance, m_farDistance);
    m_rotation.y = std::atan2(direction.x, direction.z);
    m_elapsed += dt;
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    m_position.y = groundY + m_hoverHeight + std::sin(m_elapsed * 2.5f) * 4.0f;

    const auto playerSphere = player->GetCollisionSphere();
    for (auto& shot : m_shots) {
        const Vector3 before = shot.position;
        shot.position += shot.velocity * (std::min)(dt, shot.remaining);
        shot.remaining -= dt;
        const float radius = playerSphere.radius + ShotRadius;
        // 移動区間全体で判定し、低FPSでもプレイヤーを飛び越えない。
        if (Collision::DistanceSquaredPointToSegment(playerSphere.center, {before, shot.position}) <=
            radius * radius) {
            player->TakeDamage(m_shotDamage);
            shot.remaining = 0;
        }
        if (shot.position.y <= groundY + ShotRadius) {
            shot.remaining = 0;
        }
    }
    std::erase_if(m_shots, [](const SpiritShot& shot) { return shot.remaining <= 0; });
    if (!GetSupportTargets().empty()) {
        m_shotCooldown = m_shotInterval;
        return;
    }
    GameTime::CountDown(m_shotCooldown, dt);
    Vector3 aim = playerSphere.center - m_position;
    if (player->GetHp() > 0 && m_shotCooldown <= 0 && aim.LengthSquared() <= m_shotRange * m_shotRange) {
        if (aim.LengthSquared() < 0.0001f) {
            aim = direction;
        }
        aim.Normalize();
        m_shots.push_back({m_position, aim * m_shotSpeed});
        m_shotCooldown = m_shotInterval;
    }
}

void CGhostEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    m_hp -= damage;
    if (m_hp <= 0) {
        m_shots.clear();
        Defeat(3, 10);
    }
}

void CGhostEnemy::Draw(Camera* camera) {
    if (m_model && !IsDead()) {
        m_model->Draw(camera, m_position, m_rotation.y);
    }
}

void CGhostEnemy::DrawEffects(Camera* camera) {
    if (!m_model || IsDead() || !GetActive()) {
        return;
    }
    for (auto target : GetSupportTargets()) {
        m_model->DrawBeam(camera, m_position, target->GetCollisionSphere().center, 3.5f,
                          Color(0.65f, 0.3f, 1, 1));
    }
    for (const auto& shot : m_shots) {
        Vector3 trail = shot.velocity;
        trail.Normalize();
        m_model->DrawBeam(camera, shot.position - trail * 3.0f, shot.position + trail * 3.0f,
                          ShotRadius * 2.0f, Color(0.3f, 0.9f, 1, 1), true);
    }
}

void CGhostEnemy::Uninit() {
    m_shots.clear();
    m_model.reset();
}

#ifdef _DEBUG
void CGhostEnemy::DrawDebugUI() {
    const auto text = [](const char8_t* value) { return reinterpret_cast<const char*>(value); };
    if (!ImGui::CollapsingHeader(text(u8"Ghostの支援設定"))) {
        return;
    }
    auto& settings = SupportSettings();
    ImGui::SliderInt(text(u8"支援対象数"), &settings.targetCount, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"支援範囲"), &settings.range, 50, 600, "%.0f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"支援対象の被ダメージ倍率"), &settings.incomingMultiplier, 0.1f, 1.0f, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"支援対象の与ダメージ倍率"), &settings.outgoingMultiplier, 1, 3, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::Button(text(u8"支援設定を初期値に戻す"))) {
        settings = GhostSupport::Settings{};
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty()) {
        return;
    }
    auto player = players[0];
    bool bossActive = false;
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        bossActive |= battle->IsBattleActive();
    }
    ImGui::BeginDisabled(bossActive || player->GetHp() <= 0);
    const float angle = Game::GetInstance()->GetCamera()->GetCameraDirection();
    const Vector3 forward(-std::sin(angle), 0, -std::cos(angle));
    if (ImGui::Button(text(u8"Ghostを正面に出す"))) {
        auto ghost = Game::GetInstance()->AddObject<CGhostEnemy>();
        const auto position = player->GetPosition() + forward * 220.0f;
        ghost->SetPosition(position.x, ghost->GetPosition().y, position.z);
    }
    if (ImGui::Button(text(u8"支援確認用スライムを4体出す"))) {
        const Vector3 side(forward.z, 0, -forward.x);
        for (int i = 0; i < 4; ++i) {
            auto enemy = Game::GetInstance()->AddObject<CEnemy>();
            const auto position = player->GetPosition() + forward * 250.0f + side * (float(i) - 1.5f) * 45.0f;
            enemy->SetPosition(position.x, enemy->GetPosition().y, position.z);
        }
    }
    ImGui::EndDisabled();
    CGhostEnemy* nearest = nullptr;
    float nearestDistance = 1e20f;
    for (auto ghost : Game::GetInstance()->GetObjects<CGhostEnemy>()) {
        const float distance = (ghost->GetPosition() - player->GetPosition()).LengthSquared();
        if (!ghost->IsDead() && distance < nearestDistance) {
            nearest = ghost;
            nearestDistance = distance;
        }
    }
    if (nearest) {
        ImGui::Text(text(u8"一番近いGhost: HP %d / 支援 %d体 / 霊弾 %d発"), nearest->m_hp,
                    static_cast<int>(nearest->GetSupportTargets().size()),
                    static_cast<int>(nearest->m_shots.size()));
        if (ImGui::Button(text(u8"Ghostを倒して支援解除を確認"))) {
            int damage = 10000;
            nearest->OnHit(damage);
        }
    }
    ImGui::TextWrapped("%s", text(u8"設定は全Ghost共通・実行中のみ有効。Ghost同士は支援せず、倍率は重複しませ"
                                  u8"ん。被ダメージは切り捨て（最低1）、与ダメージは切り上げ。"));
}
#endif
