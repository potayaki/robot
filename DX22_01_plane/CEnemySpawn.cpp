#include "CEnemySpawn.h"
#include"CEnemy.h"
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

        // 敵の出現位置をランダムに決定
        float angle = static_cast<float>(rand()) / RAND_MAX * DirectX::XM_2PI; // 0から2πまでのランダムな角度
        float radius = m_RadiusMin + static_cast<float>(rand()) / RAND_MAX * (m_RadiusMax - m_RadiusMin); // 半径をランダムに決定

        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // 敵を生成
        CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
        enemy->SetPosition(x, -3.0f, z);
        enemy->SetScale(1.0f, 1.0f, 1.0f);
    }

}

void CEnemySpawn::Draw(Camera* cam) {
    //スポナーは描画しないので、ここでは何もしない
}

void CEnemySpawn::Uninit() {
}


