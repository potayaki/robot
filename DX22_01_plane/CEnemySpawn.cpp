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
#include "CPlayer.h"
#include "Game.h"
#include <algorithm>

CEnemySpawn::CEnemySpawn() {}

CEnemySpawn::~CEnemySpawn() {}

void CEnemySpawn::Init() {
    m_ghostResources = GhostModel::Acquire();
    m_boarResources = std::make_unique<BoarModel>();
    if (!m_boarResources->Init()) {
        m_boarResources.reset();
    }
}

void CEnemySpawn::Update() {
    // ボス戦中の待ち時間をためない。解除後は通常の間隔を待ってから雑魚を出す。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        if (battle->IsBattleActive()) {
            m_spawnElapsed = 0;
            return;
        }
    }
#ifdef _DEBUG
    if (!m_debugSpawnEnabled) {
        return; // 停止中の経過時間をためず、再開時の連続生成を防ぐ。
    }
#endif
    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        return;
    }
    // 生存時間に応じて出現間隔を短くする。選択画面で止めた時間は加算されない。
    const int wave = static_cast<int>(Game::GetInstance()->m_gameTime / m_waveSeconds);
    const float interval =
        (std::max)(m_minSpawnInterval, m_spawnInterval - wave / m_intervalReductionDivisor);
    m_spawnElapsed += GameTime::DeltaSeconds();
    if (m_spawnElapsed >= interval) {
        m_spawnElapsed -= interval; // 間隔を越えた端数も次回へ持ち越す。

        // 飛行型も含めた敵の総数を制限し、機体パーツの描画負荷が増え続けないようにする。
        int livingEnemies = 0;
        for (EnemyTarget* enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy != nullptr && !enemy->IsDead()) {
                ++livingEnemies;
            }
        }
        if (livingEnemies >= m_maxEnemyCount) {
            return;
        }

        // 敵の出現位置をランダムに決定
        float angle = static_cast<float>(rand()) / RAND_MAX * DirectX::XM_2PI; // 0から2πまでのランダムな角度
        float radius = m_radiusMin + static_cast<float>(rand()) / RAND_MAX *
                                         (m_radiusMax - m_radiusMin); // 半径をランダムに決定

        // プレイヤーを中心に出現させ、移動しても戦闘が途切れないようにする。
        const auto playerPos = players[0]->GetPosition();
        float x = playerPos.x + radius * cos(angle);
        float z = playerPos.z + radius * sin(angle);

        // 突進型Boarも20%の確率で混ぜる。全てEnemyTargetなので武器と出現上限を共有する。
        const int kind = rand() % 100;
        if (kind < m_boarThreshold && m_boarResources) {
            auto boar = Game::GetInstance()->AddObject<CBoarEnemy>();
            // Initで足元に合わせた高さは保ち、出現するXZ座標だけを変更する。
            boar->SetPosition(x, boar->GetPosition().y, z);
        } else if (kind < m_mushroomThreshold) {
            auto mushroom = Game::GetInstance()->AddObject<CMushroomEnemy>();
            mushroom->SetPosition(x, mushroom->GetPosition().y, z);
        } else if (kind < m_ghostThreshold && m_ghostResources) {
            auto ghost = Game::GetInstance()->AddObject<CGhostEnemy>();
            ghost->SetPosition(x, ghost->GetPosition().y, z);
        } else if (kind < m_droneThreshold) {
            CFlyingEnemy* drone = Game::GetInstance()->AddObject<CFlyingEnemy>();
            drone->SetPosition(x, m_droneSpawnHeight, z);
        } else {
            CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
            if (kind < m_runnerThreshold) {
                enemy->SetVariant(SlimeVariant::Runner);
            } else if (kind < m_bruteThreshold && wave > 0) {
                enemy->SetVariant(SlimeVariant::Brute);
            }
            enemy->SetPosition(x, m_slimeSpawnHeight, z);
            enemy->SetScale(1.0f, 1.0f, 1.0f);
        }
    }
}

void CEnemySpawn::Draw(Camera* cam) {
    //スポナーは描画しないので、ここでは何もしない
}

void CEnemySpawn::Uninit() {
    m_ghostResources.reset();
    m_boarResources.reset();
}
