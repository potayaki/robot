/// @file CBossBattle.cpp
/// @brief 通常戦とボス戦の切り替え、バリア、ボスHP表示。

#include "CBossBattle.h"
#include "CBossEnemy.h"
#include "CPlayer.h"
#include "Game.h"
#include "Ground.h"
#include "imgui.h"
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    constexpr int BarrierSegments = 96;

    const char* Text(const char8_t* text) {
        return reinterpret_cast<const char*>(text);
    }
} // namespace

void CBossBattle::Init() {
    if (!m_white.Load("assets/texture/white.png")) {
        return;
    }
    m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
    // 壁面、下の輪、上の輪、縦線を各6頂点で描く。
    m_vertices.resize(BarrierSegments * 24);
    m_buffer.Create(m_vertices);

    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    m_ready = SUCCEEDED(
        Renderer::GetDevice()->CreateRasterizerState(&rasterizer, m_rasterizer.ReleaseAndGetAddressOf()));
}

CBossEnemy* CBossBattle::GetBoss() const {
    // 生ポインタを保存しない。撃破・シーン切り替え後に削除済みの敵を参照しないため。
    for (auto boss : Game::GetInstance()->GetObjects<CBossEnemy>()) {
        if (!boss->IsDead()) {
            return boss;
        }
    }
    return nullptr;
}

void CBossBattle::RemoveNormalEnemies() {
    for (auto enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
        if (dynamic_cast<CBossEnemy*>(enemy) == nullptr) {
            // 退場は撃破ではない。経験値・撃破数・プレゼントを増やさない。
            enemy->Destroy();
        }
    }
}

float CBossBattle::GetBossSpawnDistance() const {
    // ボスの半径35と壁際の余裕15を残す。開始直後の接触を避けるため最低70離す。
    const float maximum = (std::max)(70.0f, m_arenaRadius - 50.0f);
    return std::clamp(m_bossSpawnDistance, 70.0f, maximum);
}

void CBossBattle::BeginBattle(CPlayer* player) {
    if (IsBattleActive() || !player || player->GetHp() <= 0) {
        return;
    }
    m_progress.Begin();
    m_victoryNotice = 0;
    m_position = player->GetPosition(); // 開始地点を固定し、プレイヤーについてこないバリアにする。
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    m_position.y = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    RemoveNormalEnemies();

    // カメラの前方に出す。開始直後からプレイヤーと接触しない距離を確保する。
    // View行列は直前のUI描画で2D用になり得るため、移動と同じカメラ角度を使う。
    const float angle = Game::GetInstance()->GetCamera()->GetCameraDirection();
    const Vector3 forward(-std::sin(angle), 0, -std::cos(angle));
    const float spawnDistance = GetBossSpawnDistance();
    const Vector3 position = m_position + forward * spawnDistance + Vector3(0, 35.0f, 0);
    auto boss = Game::GetInstance()->AddObject<CBossEnemy>();
    boss->Configure(m_bossHp, position);
}

void CBossBattle::Update() {
    GameTime::CountDown(m_victoryNotice, GameTime::DeltaSeconds());
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    CPlayer* player = players.empty() ? nullptr : players[0];
    if (!player || player->GetHp() <= 0) {
        if (IsBattleActive()) {
            // 敗北時は報酬なしで解除。Debugで復活しても古いバリアやボスを残さない。
            if (auto boss = GetBoss()) {
                boss->Destroy();
            }
            m_progress.Finish(player ? player->GetKills() : 0);
        }
#ifdef _DEBUG
        m_requestStart = false;
        m_requestDefeat = false;
#endif
        return;
    }

    if (IsBattleActive()) {
        RemoveNormalEnemies();
        // 通常の撃破はOnBossDefeatedで即時処理する。外部からの削除でも閉じ込めない。
        if (!GetBoss()) {
            m_progress.Finish(player->GetKills());
        }
    }

    bool start = m_progress.ShouldStart(player->GetKills());
#ifdef _DEBUG
    start = start || m_requestStart;
    m_requestStart = false;
    if (m_requestDefeat) {
        if (auto boss = GetBoss()) {
            int damage = boss->GetMaxHp();
            boss->OnHit(damage);
        }
        m_requestDefeat = false;
    }
#endif
    if (start && !IsBattleActive()) {
        BeginBattle(player);
    }
}

void CBossBattle::OnBossDefeated() {
    if (!IsBattleActive()) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    m_progress.Finish(players.empty() ? 0 : players[0]->GetKills());
    m_victoryNotice = 4.0f;
    m_defeatedSlimeKing = true;
}

Vector3 CBossBattle::ConstrainPosition(const Vector3& position, float bodyRadius) const {
    return IsBattleActive() ? BossArena::ClampPosition(position, m_position, m_arenaRadius, bodyRadius)
                            : position;
}

void CBossBattle::DrawUI() {
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        return;
    }
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, 174), 0, ImVec2(0.5f, 0));
    ImGui::SetNextWindowSize(ImVec2((std::min)(560.0f, display.x * 0.48f), 0));
    ImGui::SetNextWindowBgAlpha(0.85f);
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("BossStatus", nullptr, flags);
    if (IsBattleActive()) {
        ImGui::TextColored(ImVec4(1, 0.72f, 0.3f, 1), "%s", Text(u8"BOSS : スライムキング"));
        if (auto boss = GetBoss()) {
            char hp[64];
            snprintf(hp, sizeof(hp), "%d / %d", boss->GetHp(), boss->GetMaxHp());
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.16f, 0.12f, 1));
            ImGui::ProgressBar(static_cast<float>(boss->GetHp()) / boss->GetMaxHp(), ImVec2(-1, 20), hp);
            ImGui::PopStyleColor();
            const auto phase = boss->GetPhase();
            const char8_t* notice = u8"バリアの中でボスを倒せ！";
            if (phase == CBossEnemy::Phase::Windup) {
                notice = u8"突進の予備動作！ 横へ避けよう";
            } else if (phase == CBossEnemy::Phase::Charge) {
                notice = u8"突進中！";
            } else if (phase == CBossEnemy::Phase::Recover) {
                notice = u8"ボスが休んでいる！ 攻撃のチャンス";
            }
            ImGui::TextWrapped("%s", Text(notice));
        }
    } else if (m_victoryNotice > 0) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0.7f, 1), "%s", Text(u8"ボス撃破！ バリア解除"));
    } else {
        ImGui::Text(Text(u8"次のボスまで あと %d 体"), m_progress.RemainingKills(players[0]->GetKills()));
    }
    ImGui::End();
}

#ifdef _DEBUG
void CBossBattle::DrawDebugUI() {
    if (!ImGui::CollapsingHeader(Text(u8"ボス戦の設定"))) {
        return;
    }
    // 戦闘中にバリアや最大HPが飛び変わらないよう、設定は次の戦闘前だけ編集する。
    ImGui::BeginDisabled(IsBattleActive());
    ImGui::SliderInt(Text(u8"出現までの撃破数"), &m_progress.m_killsPerBoss, 1, 500, "%d",
                     ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderInt(Text(u8"ボス最大HP"), &m_bossHp, 1, 5000, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(Text(u8"バリア半径"), &m_arenaRadius, 140.0f, 800.0f, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(Text(u8"ボス開始距離"), &m_bossSpawnDistance, 70.0f, 750.0f, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp);
    // 半径を小さくした場合も希望値は保持し、実際に使う距離を別に表示する。
    ImGui::Text(Text(u8"実際の開始距離：%.0f（バリア内に制限）"), GetBossSpawnDistance());
    if (ImGui::Button(Text(u8"ボス戦を開始（テスト）"))) {
        m_requestStart = true;
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!IsBattleActive());
    if (ImGui::Button(Text(u8"ボスを倒す（テスト）"))) {
        m_requestDefeat = true;
    }
    ImGui::EndDisabled();
    if (IsBattleActive()) {
        int livingEnemies = 0;
        for (auto enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (!enemy->IsDead()) {
                ++livingEnemies;
            }
        }
        ImGui::Text(Text(u8"生存中の敵 %d 体（ボス含む）"), livingEnemies);
        const auto players = Game::GetInstance()->GetObjects<CPlayer>();
        if (!players.empty()) {
            Vector3 offset = players[0]->GetPosition() - m_position;
            offset.y = 0;
            ImGui::Text(Text(u8"中心から %.1f / 移動限界 %.1f"), offset.Length(),
                        m_arenaRadius - players[0]->GetCollisionSphere().radius);
        }
    }
    ImGui::TextWrapped(
        "%s", Text(u8"設定はこのプレイ中のみ有効です。一時停止中のテスト操作は再開時に実行します。"));
}
#endif

void CBossBattle::DrawBarrier(Camera* cam) {
    if (!IsBattleActive() || !m_ready || !cam) {
        return;
    }
    const float bottom = m_position.y + 0.5f;
    const float top = bottom + 100.0f;
    const auto point = [&](float angle, float radius, float height) {
        return Vector3(m_position.x + std::cos(angle) * radius, height,
                       m_position.z + std::sin(angle) * radius);
    };
    const auto quad = [&](int index, const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d,
                          const Color& color) {
        const Vector3 corners[] = {a, b, c, c, b, d};
        for (int i = 0; i < 6; ++i) {
            auto& vertex = m_vertices[index + i];
            vertex.position = corners[i];
            vertex.color = color;
            vertex.uv = Vector2(0.5f, 0.5f);
        }
    };
    for (int i = 0; i < BarrierSegments; ++i) {
        const float a = DirectX::XM_2PI * i / BarrierSegments;
        const float b = DirectX::XM_2PI * (i + 1) / BarrierSegments;
        const float r = m_arenaRadius;
        // 半透明の壁と明るい輪で境界を示し、壁越しでもボスを見失わない濃さにする。
        quad(i * 24, point(a, r, bottom), point(b, r, bottom), point(a, r, top), point(b, r, top),
             Color(0.15f, 0.65f, 1, 0.055f));
        quad(i * 24 + 6, point(a, r - 2, bottom), point(b, r - 2, bottom), point(a, r + 2, bottom),
             point(b, r + 2, bottom), Color(0.2f, 0.7f, 1, 0.8f));
        quad(i * 24 + 12, point(a, r, top - 1), point(b, r, top - 1), point(a, r, top + 1),
             point(b, r, top + 1), Color(0.2f, 0.7f, 1, 0.35f));
        quad(i * 24 + 18, point(a, r, bottom), point(a + 0.004f, r, bottom), point(a, r, top),
             point(a + 0.004f, r, top), Color(0.2f, 0.7f, 1, i % 4 == 0 ? 0.3f : 0.0f));
    }
    m_buffer.Modify(m_vertices);

    // レーザー同様にGPU状態を退避し、バリアの加算合成を後続のUIに残さない。
    auto context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;
    float factors[4]{};
    UINT mask = 0, stencil = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->OMGetBlendState(blend.GetAddressOf(), factors, &mask);
    context->OMGetDepthStencilState(depth.GetAddressOf(), &stencil);
    context->RSGetState(rasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&topology);

    cam->SetCamera();
    Matrix world = Matrix::Identity;
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    Renderer::SetBlendState(BS_ADDITIVE);
    Renderer::SetDepthEnable(false); // 深度テストは維持し、壁で深度を書き込まない。
    context->RSSetState(m_rasterizer.Get());
    m_shader.SetGPU();
    m_white.SetGPU();
    m_buffer.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(m_vertices.size()), 0);

    context->OMSetBlendState(blend.Get(), factors, mask);
    context->OMSetDepthStencilState(depth.Get(), stencil);
    context->RSSetState(rasterizer.Get());
    context->IASetPrimitiveTopology(topology);
}

void CBossBattle::Uninit() {
    m_ready = false;
    m_rasterizer.Reset();
    m_buffer = VertexBuffer<VERTEX_3D>{};
    m_white = Texture{};
    m_shader = Shader{};
    m_vertices.clear();
}
