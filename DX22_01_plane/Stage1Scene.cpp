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
	player->SetPositin(0.0f,-3.0f,0.0f)->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(player);

	Ground* plane = Game::GetInstance()->AddObject<Ground>();
	plane->SetPositin(0.0f, -5.0f, 0.0f)->SetScale(5000.0f, 5000.0f, 5000.0f);
	m_MySceneObjects.push_back(plane);
	
	CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
	enemy->SetPositin(0.0f, -3.0f, 20.0f)->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(enemy);

   m_BulletCrosshair = Game::GetInstance()->AddObject<Texture2D>();
   m_BulletCrosshair->SetTexture("assets/texture/crosshair061.png");
   m_BulletCrosshair->SetScale(CrosshairSize, CrosshairSize, 1.0f);
   m_MySceneObjects.push_back(m_BulletCrosshair);

   m_MissileCrosshair = Game::GetInstance()->AddObject<Texture2D>();
   m_MissileCrosshair->SetTexture("assets/texture/crosshair131.png");
   m_MissileCrosshair->SetScale(CrosshairSize * 3, CrosshairSize * 3, 1.0f);
   m_MySceneObjects.push_back(m_MissileCrosshair);

}


//更新
void Stage1Scene::Update() {
    if (m_BulletCrosshair) {
        auto mouse = Input::GetMousePosition();
        m_BulletCrosshair->SetPositin(mouse.x, mouse.y, 0.0f);
    }
    if (m_MissileCrosshair) {
        auto mouse = Input::GetMousePosition();
        m_MissileCrosshair->SetPositin(mouse.x, mouse.y, 0.0f);
    }
}





// 終了処理
void Stage1Scene::Uninit() {
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance()->DeleteObject(o);
	}

}

