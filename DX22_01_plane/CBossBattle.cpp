/// @file CBossBattle.cpp
/// @brief 通常戦とボス戦の切り替え、バリア、ボスHP表示。

#include "CBossBattle.h"
#include "CBossEnemy.h"
#include "CPlayer.h"
#include "CForestStage.h"
#include "ForestLayout.h"
#include "Game.h"
#include "Ground.h"
#include "GroundProjection.h"
#include "imgui.h"
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    constexpr int BarrierSegments = 96;

    const char* Text(const char8_t* text) {
        return reinterpret_cast<const char*>(text);
    }

    const char8_t* AreaName(ForestLayout::Area area) {
        constexpr const char8_t* names[] = {u8"森の入口", u8"野営地", u8"砦の中庭", u8"砦の最奥"};
        return names[static_cast<size_t>(area)];
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
    // 種類ごとの体格と壁際の余裕15を残し、開始直後の接触を避ける。
    const float maximum = (std::max)(70.0f, m_arenaRadius - BossRadius(m_roster.Next()) - 15.0f);
    return std::clamp(m_bossSpawnDistance, 70.0f, maximum);
}

void CBossBattle::BeginBattle(CPlayer* player) {
    if (IsBattleActive() || !player || player->GetHp() <= 0) {
        return;
    }
    m_progress.Begin();
    m_activeKind = m_roster.Next();
    m_victoryNotice = 0;
    m_position = player->GetPosition(); // 開始地点を固定し、プレイヤーについてこないバリアにする。
    const auto forests = Game::GetInstance()->GetObjects<CForestStage>();
    const bool forest = !forests.empty();
    m_battleArea = forest ? forests[0]->GetArea(player->GetPosition()) : ForestLayout::Area::Entrance;
    if (forest) {
        // 最大の攻撃範囲である胞子の半径まで中心を内側に置き、矩形とバリアの補正を両立する。
        constexpr float ArenaCenterMargin = 55.0f;
        m_position = ForestLayout::ConstrainRoom(m_position, ArenaCenterMargin, m_battleArea);
    }
    const Ground* ground = Ground::Current();
    m_position.y = ground ? ground->HeightAt(m_position.x, m_position.z) : ForestLayout::GroundY;
    RemoveNormalEnemies();

    // カメラの前方に出す。開始直後からプレイヤーと接触しない距離を確保する。
    // View行列は直前のUI描画で2D用になり得るため、移動と同じカメラ角度を使う。
    const float angle = Game::GetInstance()->GetCamera()->GetCameraDirection();
    const Vector3 forward(-std::sin(angle), 0, -std::cos(angle));
    const float spawnDistance = GetBossSpawnDistance();
    Vector3 position = m_position + forward * spawnDistance;
    if (forest) {
        // 壁際でカメラ正面が森の外を向く場合も、プレイヤーから離して配置する。
        const float roomLowZ = ForestLayout::RoomMinZ(m_battleArea);
        const float roomHighZ = ForestLayout::RoomMaxZ(m_battleArea);
        Vector3 inward((ForestLayout::MinX + ForestLayout::MaxX) * 0.5f - m_position.x, 0,
                       (roomLowZ + roomHighZ) * 0.5f - m_position.z);
        if (inward.LengthSquared() < 0.0001f) {
            inward = forward;
        }
        inward.Normalize();
        position = ConstrainPosition(m_position + inward * spawnDistance, BossRadius(m_activeKind));
        constexpr int SpawnDirections = 8;
        constexpr float MinimumBossSpawnDistance = 70.0f;
        for (int i = 0; i < SpawnDirections; ++i) {
            const float candidateAngle = angle + DirectX::XM_2PI * i / SpawnDirections;
            const Vector3 direction(-std::sin(candidateAngle), 0, -std::cos(candidateAngle));
            const Vector3 candidate = m_position + direction * spawnDistance;
            const Vector3 constrained = ConstrainPosition(candidate, BossRadius(m_activeKind));
            Vector3 offset = constrained - player->GetPosition();
            offset.y = 0;
            if (offset.LengthSquared() >= MinimumBossSpawnDistance * MinimumBossSpawnDistance) {
                position = constrained;
                break;
            }
        }
    }
    position.y = (ground ? ground->HeightAt(position.x, position.z) : ForestLayout::GroundY) +
                 BossHeight(m_activeKind);
    auto boss = Game::GetInstance()->AddObject<CBossEnemy>();
    boss->Configure(m_bossHp, position, m_activeKind);
    if (m_stageBossKind) {
        m_roster.SelectNext(*m_stageBossKind);
    }
#ifdef _DEBUG
    m_debugBossSelected = false;
#endif
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

    const auto forests = Game::GetInstance()->GetObjects<CForestStage>();
    CForestStage* forest = forests.empty() ? nullptr : forests[0];
    if (forest && !IsBattleActive()) {
        const auto area = forest->GetArea(player->GetPosition());
        if (area != m_currentArea) {
            m_currentArea = area;
            m_progress.Finish(player->GetKills());
            // 前の部屋の敵を持ち越さず、新しいエリアの種類と出現上限へ切り替える。
            RemoveNormalEnemies();
        }
        m_stageBossKind = forest->NextBoss(area);
#ifdef _DEBUG
        if (!m_debugBossSelected)
#endif
        {
            m_roster.SelectNext(*m_stageBossKind);
        }
    }
    const bool cleared = forest && forest->IsAreaCleared(m_currentArea);
    bool start = !cleared && m_progress.ShouldStart(player->GetKills());
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

void CBossBattle::OnBossDefeated(BossKind kind) {
    if (!IsBattleActive() || kind != m_activeKind) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    m_progress.Finish(players.empty() ? 0 : players[0]->GetKills());
    m_victoryNotice = 4.0f;
    m_roster.RecordDefeat(kind);
    for (auto forest : Game::GetInstance()->GetObjects<CForestStage>()) {
        forest->OnBossDefeated(kind, m_battleArea);
        m_stageBossKind = forest->NextBoss(m_battleArea);
        if (forest->IsComplete()) {
            RemoveNormalEnemies();
        }
    }
    if (m_stageBossKind) {
        m_roster.SelectNext(*m_stageBossKind);
    }
}

Vector3 CBossBattle::ConstrainPosition(const Vector3& position, float bodyRadius) const {
    if (!IsBattleActive()) {
        return position;
    }
    Vector3 result = position;
    if (!Game::GetInstance()->GetObjects<CForestStage>().empty()) {
        result = ForestLayout::ConstrainRoom(result, bodyRadius, m_battleArea);
    }
    return BossArena::ClampPosition(result, m_position, m_arenaRadius, bodyRadius);
}

void CBossBattle::DrawUI() {
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        return;
    }
    const auto forests = Game::GetInstance()->GetObjects<CForestStage>();
    CForestStage* forest = forests.empty() ? nullptr : forests[0];
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, 174), 0, ImVec2(0.5f, 0));
    ImGui::SetNextWindowSize(ImVec2((std::min)(560.0f, display.x * 0.48f), 0));
    ImGui::SetNextWindowBgAlpha(0.85f);
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("BossStatus", nullptr, flags);
    if (forest) {
        ImGui::TextUnformatted(Text(AreaName(forest->GetArea(players[0]->GetPosition()))));
    }
    if (IsBattleActive()) {
        ImGui::TextColored(ImVec4(1, 0.72f, 0.3f, 1), "BOSS  %s", Text(BossName(m_activeKind)));
        if (auto boss = GetBoss()) {
            char hp[64];
            snprintf(hp, sizeof(hp), "%d / %d", boss->GetHp(), boss->GetMaxHp());
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.16f, 0.12f, 1));
            ImGui::ProgressBar(static_cast<float>(boss->GetHp()) / boss->GetMaxHp(), ImVec2(-1, 20), hp);
            ImGui::PopStyleColor();
            ImGui::TextWrapped("%s", Text(boss->GetAttackNotice()));
        }
    } else if (forest && forest->IsComplete()) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0.7f, 1), "%s", Text(u8"砦を攻略した！ 全エリア制圧"));
        ImGui::TextWrapped("%s", Text(u8"森から砦の最奥まで、自由に探索できます"));
    } else if (m_victoryNotice > 0) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0.7f, 1), "%s", Text(u8"ボス撃破！ バリア解除"));
    } else if (forest && forest->IsAreaCleared(m_currentArea)) {
        const auto gateIndex = static_cast<size_t>(m_currentArea);
        if (forest->IsGateOpen(gateIndex)) {
            const auto nextArea = static_cast<ForestLayout::Area>(gateIndex + 1);
            ImGui::TextWrapped(Text(u8"門が開いた！ 奥の%sへ進もう"), Text(AreaName(nextArea)));
        } else {
            ImGui::TextWrapped("%s", Text(u8"門が開いています……"));
        }
    } else {
        ImGui::Text(Text(u8"%s まで あと %d 体"), Text(BossName(m_roster.Next())),
                    m_progress.RemainingKills(players[0]->GetKills()));
    }
    if (forest && !IsBattleActive() && !forest->IsAreaCleared(m_currentArea)) {
        const auto target = forest->NextBoss(m_currentArea);
        const char8_t* objective = m_currentArea == ForestLayout::Area::Keep
                                       ? u8"%sを倒して砦の最奥を制圧しよう"
                                       : u8"%sを倒して奥の門を開こう";
        ImGui::TextWrapped(Text(objective), Text(BossName(target)));
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
    if (ImGui::BeginCombo(Text(u8"次に出すボス"), Text(BossName(m_roster.Next())))) {
        for (int index = 0; index < BossTypeCount; ++index) {
            const auto kind = static_cast<BossKind>(index);
            if (ImGui::Selectable(Text(BossName(kind)), kind == m_roster.Next())) {
                m_roster.SelectNext(kind);
                m_debugBossSelected = true;
            }
        }
        ImGui::EndCombo();
    }
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
    const GroundProjection ground(Ground::Current(), m_position.y);
    constexpr float bottom = 0.8f;
    const Vector3 wallHeight(0, 100, 0);
    const auto point = [&](float angle, float radius) {
        return ground.Point(
            Vector3(m_position.x + std::cos(angle) * radius, 0, m_position.z + std::sin(angle) * radius),
            bottom);
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
        const Vector3 from = point(a, r);
        const Vector3 to = point(b, r);
        const Vector3 upperFrom = from + wallHeight;
        const Vector3 upperTo = to + wallHeight;
        const Vector3 rib = point(a + 0.004f, r);
        // 半透明の壁と明るい輪で境界を示し、壁越しでもボスを見失わない濃さにする。
        quad(i * 24, from, to, upperFrom, upperTo, Color(0.15f, 0.65f, 1, 0.055f));
        quad(i * 24 + 6, point(a, r - 2), point(b, r - 2), point(a, r + 2), point(b, r + 2),
             Color(0.2f, 0.7f, 1, 0.8f));
        quad(i * 24 + 12, upperFrom - Vector3::UnitY, upperTo - Vector3::UnitY, upperFrom + Vector3::UnitY,
             upperTo + Vector3::UnitY, Color(0.2f, 0.7f, 1, 0.35f));
        quad(i * 24 + 18, from, rib, upperFrom, rib + wallHeight,
             Color(0.2f, 0.7f, 1, i % 4 == 0 ? 0.3f : 0.0f));
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
