/// @file CEnemySpawn.cpp
/// @brief 経過時間に応じた敵の出現と、同時出現数の管理。

#include "CEnemySpawn.h"
#include "CEnemy.h"
#include "CFlyingEnemy.h"
#include "CBoarEnemy.h"
#include "CMushroomEnemy.h"
#include "CGhostEnemy.h"
#include "EnemyTarget.h"
#include "CBossBattle.h"
#include "CForestStage.h"
#include "ForestLayout.h"
#include "EnemySpawnRules.h"
#include "EnemyRarity.h"
#include "CPlayer.h"
#include "Game.h"
#include "Ground.h"
#include <algorithm>
#include <filesystem>
#include <utility>

CEnemySpawn::CEnemySpawn() {}

CEnemySpawn::~CEnemySpawn() {}

EnemySpawning::Pressure CEnemySpawn::GetPressure(double elapsedSeconds) const {
    return EnemySpawning::GetPressure(elapsedSeconds, m_enemyCount, m_maxEnemyCount, m_spawnInterval,
                                      m_waveSeconds, m_minSpawnInterval, m_intervalReductionDivisor);
}

void CEnemySpawn::Init() {
    m_spawnElapsed = 0.0f;
    m_slimeResources = std::make_unique<TestModel>();
    m_slimeResources->Init();
    m_slimeResources->Load("assets/model/slime/Slime.fbx", "assets/model/slime",
                           "assets/model/slime/albedo.tga", true);
    if (std::filesystem::exists("assets/model/drone/Drone.fbx")) {
        m_flyResources = std::make_unique<TestModel>();
        m_flyResources->Init();
        m_flyResources->Load("assets/model/drone/Drone.fbx", "assets/model/drone",
                             "assets/model/drone/albedo.tga", true);
    }
    m_mushroomResources = std::make_unique<MushroomModel>();
    if (!m_mushroomResources->Init()) {
        m_mushroomResources.reset();
    }
    m_ghostResources = GhostModel::Acquire();
    m_boarResources = std::make_unique<BoarModel>();
    if (!m_boarResources->Init()) {
        m_boarResources.reset();
    }
}

void CEnemySpawn::Update() {
    auto game = Game::GetInstance();
    // ボス戦中の待ち時間をためない。解除後は通常の間隔を待ってから雑魚を出す。
    for (auto battle : game->GetObjects<CBossBattle>()) {
        if (battle->IsBattleActive()) {
            m_spawnElapsed = 0;
            return;
        }
    }
#ifdef _DEBUG
    if (!m_debugSpawnEnabled) {
        m_spawnElapsed = 0;
        return; // 停止中の経過時間をためず、再開時の連続生成を防ぐ。
    }
#endif
    auto players = game->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        m_spawnElapsed = 0;
        return;
    }
    const auto forests = game->GetObjects<CForestStage>();
    CForestStage* forest = forests.empty() ? nullptr : forests[0];
    if (forest && forest->IsComplete()) {
        m_spawnElapsed = 0;
        return;
    }
    // 生存時間に応じて群れ・同時出現枠を増やす。選択画面で止めた時間は加算されない。
    const auto pressure = GetPressure(game->m_gameTime);
    if (!EnemySpawning::Advance(m_spawnElapsed, GameTime::DeltaSeconds(), pressure.interval)) {
        return;
    }

    // 追加待ちも含めて数え、同じフレームの別スポナーやデバッグ生成との上限超過を防ぐ。
    int livingEnemies = 0;
    int ghostCount = 0;
    std::vector<DirectX::SimpleMath::Vector3> occupied;
    for (EnemyTarget* enemy : game->GetObjects<EnemyTarget>(true)) {
        if (enemy != nullptr && !enemy->IsDead() && enemy->GetActive()) {
            ++livingEnemies;
            ghostCount += dynamic_cast<CGhostEnemy*>(enemy) != nullptr;
            occupied.push_back(enemy->GetPosition());
        }
    }
    const int packSize = EnemySpawning::PackSize(pressure.packSize, livingEnemies, pressure.maxAlive);
    if (packSize == 0) {
        return;
    }
    const auto playerPos = players[0]->GetPosition();
    const Ground* ground = Ground::Current();
    const auto area = forest ? forest->GetArea(playerPos) : ForestLayout::Area::Entrance;
    const int areaDepth = forest ? static_cast<int>(area) : 0;
    const int variantPercent = std::clamp(m_variantPercent + areaDepth * 5, 0, 100);
    const int rarePercent = std::clamp(m_rarePercent + areaDepth * 2, 0, 100);
    const float radiusMin = (std::max)(EnemySpawning::SafePlayerDistance, m_radiusMin);
    const float radiusMax = (std::max)(radiusMin, m_radiusMax);
    const float cameraYaw = game->GetCamera()->GetCameraDirection();
    std::vector<DirectX::SimpleMath::Vector3> pack;
    for (int attempt = 0; attempt < EnemySpawning::PositionAttempts; ++attempt) {
        // まず画面前方の約80度を探し、壁際で配置できないときは全周へ広げる。
        constexpr int forwardAttempts = 4;
        constexpr float forwardHalfAngle = DirectX::XM_PI * 0.22f;
        const float arc = attempt < forwardAttempts ? forwardHalfAngle : DirectX::XM_PI;
        const float angle = cameraYaw + (2.0f * rand() / RAND_MAX - 1.0f) * arc;
        const DirectX::SimpleMath::Vector3 forward(-std::sin(angle), 0, -std::cos(angle));
        const float radius = radiusMin + static_cast<float>(rand()) / RAND_MAX * (radiusMax - radiusMin);
        auto positions = EnemySpawning::PlacePack(playerPos + forward * radius, forward, packSize, playerPos,
                                                  occupied, forest != nullptr, area);
        if (positions.size() > pack.size()) {
            pack = std::move(positions);
            if (pack.size() == static_cast<size_t>(packSize)) {
                break;
            }
        }
    }
    for (const auto& spawnPosition : pack) {
        const float x = spawnPosition.x;
        const float z = spawnPosition.z;
        const float groundY = ground ? ground->HeightAt(x, z) : ForestLayout::GroundY;

        // エリアの構成を既存の種類選択の範囲へ振り分ける。
        int kind = m_slimesOnly ? m_droneThreshold + rand() % (100 - m_droneThreshold) : rand() % 100;
        if (forest) {
            switch (area) {
                case ForestLayout::Area::Entrance:
                    kind = m_droneThreshold + rand() % (100 - m_droneThreshold);
                    break;
                case ForestLayout::Area::Camp:
                    kind = m_ghostThreshold;
                    break;
                case ForestLayout::Area::Courtyard:
                    kind = rand() % 100 < 75 && m_boarResources ? 0 : m_boarThreshold;
                    break;
                case ForestLayout::Area::Keep: {
                    // 最初はキノコを出し、支援型の比率を制限してからGhostを混ぜる。
                    kind = EnemySpawning::CanSpawnGhost(livingEnemies, ghostCount) && m_ghostResources &&
                                   rand() % 100 < 35
                               ? m_mushroomThreshold
                               : m_boarThreshold;
                    break;
                }
            }
        }
        if (kind >= m_mushroomThreshold && kind < m_ghostThreshold &&
            !EnemySpawning::CanSpawnGhost(livingEnemies, ghostCount)) {
            kind = m_boarThreshold;
        }
        EnemyTarget* spawned = nullptr;
        if (kind < m_boarThreshold && m_boarResources) {
            auto boar = game->AddObject<CBoarEnemy>();
            boar->SetPosition(x, groundY + boar->GetCollisionSphere().radius, z);
            spawned = boar;
        } else if (kind < m_mushroomThreshold) {
            auto mushroom = game->AddObject<CMushroomEnemy>();
            mushroom->SetPosition(x, groundY + mushroom->GetCollisionSphere().radius, z);
            spawned = mushroom;
        } else if (kind < m_ghostThreshold && m_ghostResources) {
            auto ghost = game->AddObject<CGhostEnemy>();
            ghost->SetPosition(x, groundY + CGhostEnemy::HoverHeight, z);
            spawned = ghost;
        } else if (kind < m_droneThreshold) {
            CFlyingEnemy* drone = game->AddObject<CFlyingEnemy>();
            drone->SetPosition(x, groundY + m_droneSpawnHeight, z);
            spawned = drone;
        } else {
            CEnemy* enemy = game->AddObject<CEnemy>();
            if (kind < m_runnerThreshold) {
                enemy->SetVariant(SlimeVariant::Runner);
            } else if (kind < m_bruteThreshold && pressure.stage > 0) {
                enemy->SetVariant(SlimeVariant::Brute);
            }
            const float feet = ground ? groundY : m_slimeSpawnHeight;
            enemy->SetPosition(x, feet + enemy->GetCollisionSphere().radius, z);
            enemy->SetScale(1.0f, 1.0f, 1.0f);
            spawned = enemy;
        }
        if (spawned && !spawned->IsDead()) {
            auto rarity = EnemyVariants::Choose(rand() % 100, variantPercent, rarePercent);
#ifdef _DEBUG
            if (m_debugRarity >= 0) {
                rarity = static_cast<EnemyRarity>(std::clamp(m_debugRarity, 0, 2));
            }
#endif
            spawned->SetRarity(rarity);
            ++livingEnemies;
            ghostCount += dynamic_cast<CGhostEnemy*>(spawned) != nullptr;
        }
    }
}

void CEnemySpawn::Draw(Camera* cam) {
    //スポナーは描画しないので、ここでは何もしない
}

void CEnemySpawn::Uninit() {
    m_slimeResources.reset();
    m_flyResources.reset();
    m_mushroomResources.reset();
    m_ghostResources.reset();
    m_boarResources.reset();
}
