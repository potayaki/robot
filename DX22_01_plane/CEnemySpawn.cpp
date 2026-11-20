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

CEnemySpawn::CEnemySpawn() {}

CEnemySpawn::~CEnemySpawn() {}

EnemySpawning::Pressure CEnemySpawn::GetPressure(double elapsedSeconds) const {
    return EnemySpawning::GetPressure(elapsedSeconds, m_enemyCount, m_maxEnemyCount, m_spawnInterval,
                                      m_waveSeconds, m_minSpawnInterval, m_intervalReductionDivisor);
}

void CEnemySpawn::Init() {
    m_spawnElapsed = 0.0f; // 初期配置の3体から始め、追加の敵は出現間隔を待つ。
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
    const auto playerPos = players[0]->GetPosition();
    const auto grounds = game->GetObjects<Ground>();
    const float groundY = grounds.empty() ? m_slimeSpawnHeight : grounds[0]->GetPosition().y;
    const auto area = forest ? forest->GetArea(playerPos) : ForestLayout::Area::Entrance;
    const int areaDepth = forest ? static_cast<int>(area) : 0;
    const int variantPercent = std::clamp(m_variantPercent + areaDepth * 5, 0, 100);
    const int rarePercent = std::clamp(m_rarePercent + areaDepth * 2, 0, 100);
    const float radiusMin = (std::max)(EnemySpawning::SafePlayerDistance, m_radiusMin);
    const float radiusMax = (std::max)(radiusMin, m_radiusMax);
    for (int member = 0; member < packSize; ++member) {
        // 各個体の場所を個別に抽選し、同じ群れと既存の敵の両方から間隔を取る。
        DirectX::SimpleMath::Vector3 spawnPosition;
        bool foundPosition = false;
        for (int attempt = 0; attempt < EnemySpawning::PositionAttempts; ++attempt) {
            const float angle = static_cast<float>(rand()) / RAND_MAX * DirectX::XM_2PI;
            const float radius = radiusMin + static_cast<float>(rand()) / RAND_MAX * (radiusMax - radiusMin);
            spawnPosition =
                playerPos + DirectX::SimpleMath::Vector3(radius * cos(angle), 0, radius * sin(angle));
            if (!EnemySpawning::IsClearPosition(spawnPosition, playerPos, occupied, forest != nullptr,
                                                area)) {
                continue;
            }
            foundPosition = true;
            break;
        }
        if (!foundPosition) {
            continue;
        }
        const float x = spawnPosition.x;
        const float z = spawnPosition.z;

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
            // Initで足元に合わせた高さは保ち、出現するXZ座標だけを変更する。
            boar->SetPosition(x, boar->GetPosition().y, z);
            spawned = boar;
        } else if (kind < m_mushroomThreshold) {
            auto mushroom = game->AddObject<CMushroomEnemy>();
            mushroom->SetPosition(x, mushroom->GetPosition().y, z);
            spawned = mushroom;
        } else if (kind < m_ghostThreshold && m_ghostResources) {
            auto ghost = game->AddObject<CGhostEnemy>();
            ghost->SetPosition(x, ghost->GetPosition().y, z);
            spawned = ghost;
        } else if (kind < m_droneThreshold) {
            CFlyingEnemy* drone = game->AddObject<CFlyingEnemy>();
            drone->SetPosition(x, m_droneSpawnHeight, z);
            spawned = drone;
        } else {
            CEnemy* enemy = game->AddObject<CEnemy>();
            if (kind < m_runnerThreshold) {
                enemy->SetVariant(SlimeVariant::Runner);
            } else if (kind < m_bruteThreshold && pressure.stage > 0) {
                enemy->SetVariant(SlimeVariant::Brute);
            }
            enemy->SetPosition(x, groundY + enemy->GetCollisionSphere().radius, z);
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
            occupied.push_back(spawned->GetPosition());
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
