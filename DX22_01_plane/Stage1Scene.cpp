#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Ground.h"
#include"Arrow.h"
#include"Pole.h"
#include"Texture2D.h"
#include"CPlayer.h"
#include"Collision.h"
#include"CEnemy.h"
#include<cmath>
#include"CBullet.h"
#include"billboard.h"
#include"CParticle.h"
#include"CPlayerUI.h"
using namespace DirectX::SimpleMath;

#define CrosshairSize 64.0f  // クロスヘアのサイズ

// コンストラクタ
Stage1Scene::Stage1Scene() {
	Init();
}

// デストラクタ
Stage1Scene::~Stage1Scene() {
	Uninit();
}

// 初期化
void Stage1Scene::Init() {
	
	CPlayer* player = Game::GetInstance()->AddObject<CPlayer>();
	player->SetPosition(0.0f,-3.0f,0.0f)->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(player);

	Ground* plane = Game::GetInstance()->AddObject<Ground>();
	plane->SetPosition(0.0f, -5.0f, 0.0f)->SetScale(5000.0f, 5000.0f, 5000.0f);
	m_MySceneObjects.push_back(plane);
	
	CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
    enemy->SetPosition(0.0f, -3.0f, 20.0f);
    enemy->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(enemy);

   m_BulletCrosshair = Game::GetInstance()->AddUI<Texture2D>();
   m_BulletCrosshair->SetTexture("assets/texture/crosshair061.png");
   m_BulletCrosshair->SetScale(CrosshairSize, CrosshairSize, 10000.0f);
   m_MySceneObjects.push_back(m_BulletCrosshair);

   m_MissileCrosshair = Game::GetInstance()->AddUI<Texture2D>();
   m_MissileCrosshair->SetTexture("assets/texture/crosshair1310.png");
   m_MissileCrosshair->SetScale(CrosshairSize * 4, CrosshairSize * 4, 10000.0f);
   m_MySceneObjects.push_back(m_MissileCrosshair);

   billboard::LoadTextures("assets/texture/PNG/Smoke/Smoke_Frame_", ".png", 10); // 10枚の画像を読み込む

   CParticle::Preload(); // パーティクル用のモデルを事前に読み込む

   CPlayerUI* CycleUI = Game::GetInstance()->AddUI<CPlayerUI>();
   CycleUI->SetPosition(-400.0f, -600.0f, 0.0f);
   CycleUI->SetScale(200.0f, 200.0f, 1.0f);

}


//更新
void Stage1Scene::Update() {
    if (m_BulletCrosshair) {
        auto mouse = Input::GetMousePosition();
        m_BulletCrosshair->SetPosition(mouse.x, mouse.y, 0.0f);
    }
    if (m_MissileCrosshair) {
        auto mouse = Input::GetMousePosition();
        m_MissileCrosshair->SetPosition(mouse.x, mouse.y, 0.0f);
    }
}





// 終了処理
void Stage1Scene::Uninit() {
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
        o->Destroy();
	}

}

