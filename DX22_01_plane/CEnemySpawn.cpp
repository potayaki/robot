#include "CEnemySpawn.h"
#include"CEnemy.h"
#include "CFlyingEnemy.h"
#include "EnemyTarget.h"
#include "CBossBattle.h"
#include"CPlayer.h"
#include"Game.h"
#include <algorithm>

CEnemySpawn::CEnemySpawn() {
}

CEnemySpawn::~CEnemySpawn() {
}

void CEnemySpawn::Init() {
}

void CEnemySpawn::Update() {
    // ボス戦中の待ち時間をためない。解除後は通常の間隔を待ってから雑魚を出す。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>())
    {
        if (battle->IsBattleActive())
        {
            m_spawnElapsed = 0;
            return;
        }
    }
#ifdef _DEBUG
    if (!m_debugSpawnEnabled) return; // 停止中の経過時間をためず、再開時の連続生成を防ぐ。
#endif
    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) return;
    // 生存時間に応じて出現間隔を短くする。選択画面で止めた時間は加算されない。
    const int wave = static_cast<int>(Game::GetInstance()->m_gameTime / 45.0f);
    const float interval = (std::max)(0.75f, m_SpawnInterval - wave / 6.0f);
    m_spawnElapsed += GameTime::DeltaSeconds();
    if (m_spawnElapsed >= interval) {
        m_spawnElapsed -= interval; // 間隔を越えた端数も次回へ持ち越す。

        // 飛行型も含めた敵の総数を制限し、機体パーツの描画負荷が増え続けないようにする。
        int livingEnemies = 0;
        for (EnemyTarget* enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy != nullptr && !enemy->IsDead()) ++livingEnemies;
        }
        if (livingEnemies >= m_MaxEnemyCount) return;

        // 敵の出現位置をランダムに決定
        float angle = static_cast<float>(rand()) / RAND_MAX * DirectX::XM_2PI; // 0から2πまでのランダムな角度
        float radius = m_RadiusMin + static_cast<float>(rand()) / RAND_MAX * (m_RadiusMax - m_RadiusMin); // 半径をランダムに決定

        // プレイヤーを中心に出現させ、移動しても戦闘が途切れないようにする。
        const auto playerPos = players[0]->GetPosition();
        float x = playerPos.x + radius * cos(angle);
        float z = playerPos.z + radius * sin(angle);

        // 飛行型・高速型・通常型を混ぜ、45秒以降は大型も候補に加える。
        const int kind = rand() % 100;
        if (kind < 20) {
            CFlyingEnemy* drone = Game::GetInstance()->AddObject<CFlyingEnemy>();
            drone->SetPosition(x, 40.0f, z);
        }
        else {
            CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
            if (kind < 45) enemy->SetVariant(SlimeVariant::Runner);
            else if (kind < 60 && wave > 0) enemy->SetVariant(SlimeVariant::Brute);
            enemy->SetPosition(x, -3.0f, z);
            enemy->SetScale(1.0f, 1.0f, 1.0f);
        }
    }

}

void CEnemySpawn::Draw(Camera* cam) {
    //スポナーは描画しないので、ここでは何もしない
}

void CEnemySpawn::Uninit() {
}


