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
#include<cmath>
using namespace DirectX::SimpleMath;

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
	player->Init();
	player->SetPositin(0.0f,-3.0f,0.0f)->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(player);

	Ground* plane = Game::GetInstance()->AddObject<Ground>();
	plane->Init();
	plane->SetPositin(0.0f, -5.0f, 0.0f)->SetScale(5000.0f, 5000.0f, 5000.0f);
	m_MySceneObjects.push_back(plane);
	
}

//更新
void Stage1Scene::Update() {
	for (auto& a : m_MySceneObjects) {
		a->Update();
	}
}





// 終了処理
void Stage1Scene::Uninit() {
	// このシーンのオブジェクトを削除する
	for (auto& o : m_MySceneObjects) {
		Game::GetInstance()->DeleteObject(o);
	}
	
	
	
	



}

