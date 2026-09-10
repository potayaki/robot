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
#include"PoolManager.h"
#include"CMissile.h"
#include"CMiniMap.h"
#include"CTimer.h"
#include"CEnemySpawn.h"
#include"CPresentBox.h"
#include"ExplosionManager.h"
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

    //プレゼントボックスの共有モデルの読み込み(複数生成してもOK)
    CPresentBox::LoadResource(); 

    //--------------------
    //プレイヤーの生成
    //--------------------
	CPlayer* player = Game::GetInstance()->AddObject<CPlayer>();
	player->SetPosition(0.0f,-3.0f,-500.0f)->SetScale(1.0f, 1.0f, 1.0f);
	m_MySceneObjects.push_back(player);

    
     //--------------------
    //地面の生成
   //--------------------
	Ground* plane = Game::GetInstance()->AddObject<Ground>();
	plane->SetPosition(0.0f, -5.0f, 0.0f)->SetScale(5000.0f, 5000.0f, 5000.0f);
	m_MySceneObjects.push_back(plane);


     
    //--------------------
    //敵の生成(自動で敵が出てくるので消してもよいけどDebug時に役に立つ)
//--------------------
    CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
    enemy->SetPosition(60.0f , -3.0f, 0.0f);
    enemy->SetScale(1.0f, 1.0f, 1.0f);
    m_MySceneObjects.push_back(enemy);


    
    
     //--------------------
    //　UI　小さい方のクロスヘア
   //--------------------
   m_BulletCrosshair = Game::GetInstance()->AddUI<Texture2D>();
   m_BulletCrosshair->SetTexture("assets/texture/crosshair061.png");
   m_BulletCrosshair->SetScale(CrosshairSize, CrosshairSize, 10000.0f);
   m_MySceneObjects.push_back(m_BulletCrosshair);

    //--------------------
   //　UI　大きい方のクロスヘア
  //--------------------
   m_MissileCrosshair = Game::GetInstance()->AddUI<Texture2D>();
   m_MissileCrosshair->SetTexture("assets/texture/crosshair1310.png");
   m_MissileCrosshair->SetScale(CrosshairSize * 4, CrosshairSize * 4, 10000.0f);
   m_MySceneObjects.push_back(m_MissileCrosshair);

  
   //パーティクルのモデルを事前読み込み
   CParticle::Preload(); // パーティクル用のモデルを事前に読み込む


   //--------------------
  //左下UIプレイヤーのHPとミサイルが打てるまでの時間
 //--------------------
   CPlayerUI* CycleUI = Game::GetInstance()->AddUI<CPlayerUI>();
   CycleUI->SetPosition(-530.0f, -260.0f, 0.0f);
   CycleUI->SetScale(200.0f, 200.0f, 1.0f);
   

 
   //--------------------
  //オブジェクトプールの生成
//--------------------
   ExplosionManager* explosionManager = Game::GetInstance()->AddObject<ExplosionManager>();
   m_MySceneObjects.push_back(explosionManager);
   m_MySceneObjects.push_back(Game::GetInstance()->AddObject<BulletManager>());
   m_MySceneObjects.push_back(Game::GetInstance()->AddObject<MissileManager>());
   m_MySceneObjects.push_back(Game::GetInstance()->AddObject<ParticleManager>());
   
  //--------------------
 //　UI　ミニマップ
// --------------------
   CMiniMap* miniMap = Game::GetInstance()->AddUI<CMiniMap>();
   miniMap->SetPosition(-480.0f, 250.0f, 0.0f);
   miniMap->SetScale(200.0f, 200.0f, 1.0f);
   m_MySceneObjects.push_back(miniMap);

  //--------------------
 //　UI　タイマー
// --------------------
   CTimer* timerUI = Game::GetInstance()->AddUI<CTimer>();
   timerUI->SetPosition(-50.0f,320.0f, 0.0f); // 画面中央の上部
   timerUI->SetScale(40.0f, 60.0f, 1.0f);     // 数字1桁あたりの大きさ
   m_MySceneObjects.push_back(timerUI);

 
   //--------------------
  //敵キャラのスポーン
// --------------------
   CEnemySpawn* pawner = Game::GetInstance()->AddObject<CEnemySpawn>();
   m_MySceneObjects.push_back(pawner);


}


//更新
void Stage1Scene::Update() {

    //マウス位置にクロスヘアを追従させる
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
    CPresentBox::UnloadResource(); // 共有モデルの解放
}

