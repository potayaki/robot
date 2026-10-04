#include "CEnemySpawn.h"
#include"CEnemy.h"
#include "CFlyingEnemy.h"
#include "EnemyTarget.h"
#include"CPlayer.h"
#include"Game.h"

CEnemySpawn::CEnemySpawn() {
}

CEnemySpawn::~CEnemySpawn() {
}

void CEnemySpawn::Init() {
}

void CEnemySpawn::Update() {
    FrameCount++;
    if (FrameCount >= m_SpawnInterval) {
        FrameCount = 0;

        // 飛行型も含めた敵の総数を制限し、機体パーツの描画負荷が増え続けないようにする。
        int livingEnemies = 0;
        for (EnemyTarget* enemy : Game::GetInstance()->GetObjects<EnemyTarget>()) {
            if (enemy != nullptr && !enemy->IsDead()) ++livingEnemies;
        }
        if (livingEnemies >= m_MaxEnemyCount) return;

        // 敵の出現位置をランダムに決定
        float angle = static_cast<float>(rand()) / RAND_MAX * DirectX::XM_2PI; // 0から2πまでのランダムな角度
        float radius = m_RadiusMin + static_cast<float>(rand()) / RAND_MAX * (m_RadiusMax - m_RadiusMin); // 半径をランダムに決定

        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // 4回に1回はホバードローンを出し、スライムとの混成にする。
        if (rand() % 4 == 0) {
            CFlyingEnemy* drone = Game::GetInstance()->AddObject<CFlyingEnemy>();
            drone->SetPosition(x, 40.0f, z);
        }
        else {
            CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
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


