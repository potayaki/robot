/// @file Stage1Scene.cpp
/// @brief ステージの地形、敵、UI、照準画像の初期配置。

#include "Stage1Scene.h"
#include "Game.h"
#include "Input.h"
#include "GolfBall.h"
#include "Ground.h"
#include "Arrow.h"
#include "Pole.h"
#include "Texture2D.h"
#include "CPlayer.h"
#include "Collision.h"
#include "CEnemy.h"
#include "CFlyingEnemy.h"
#include <cmath>
#include "CBullet.h"
#include "billboard.h"
#include "CParticle.h"
#include "CPlayerUI.h"
#include "PoolManager.h"
#include "CMissile.h"
#include "CMiniMap.h"
#include "CTimer.h"
#include "CEnemySpawn.h"
#include "CPresentBox.h"
#include "CSpanner.h"
#include "ExplosionManager.h"
#include "CAutoturret.h"
#include "CBossBattle.h"
#include "AimCursor.h"
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
    Game::GetInstance()->m_gameTime = 0.0f; // 新しいプレイでは敵の出現段階もリセット。

    //プレゼントボックスの共有モデルの読み込み(複数生成してもOK)
    CPresentBox::LoadResource();
    CSpanner::LoadResource();

    // プレイヤー・武器・スポナーより先に更新し、ボス開始フレームの雑魚を退場させる。
    m_mySceneObjects.push_back(Game::GetInstance()->AddObject<CBossBattle>());

    //--------------------
    //プレイヤーの生成
    //--------------------
    CPlayer* player = Game::GetInstance()->AddObject<CPlayer>();
    player->SetPosition(0.0f, -3.0f, -500.0f)->SetScale(1.0f, 1.0f, 1.0f);
    m_mySceneObjects.push_back(player);

    //--------------------
    //地面の生成
    //--------------------
    Ground* plane = Game::GetInstance()->AddObject<Ground>();
    plane->SetPosition(0.0f, -5.0f, 0.0f)->SetScale(5000.0f, 5000.0f, 5000.0f);
    m_mySceneObjects.push_back(plane);

    //--------------------
    //敵の生成(自動で敵が出てくるので消してもよいけどDebug時に役に立つ)
    //--------------------
    /*
    CEnemy* enemy = Game::GetInstance()->AddObject<CEnemy>();
    enemy->SetPosition(60.0f , -3.0f, 0.0f);
    enemy->SetScale(1.0f, 1.0f, 1.0f);
    m_MySceneObjects.push_back(enemy);

    // 開始直後から新しい敵を確認できるよう、プレイヤー前方にドローンを1機配置する。
    CFlyingEnemy* drone = Game::GetInstance()->AddObject<CFlyingEnemy>();
    drone->SetPosition(65.0f, 40.0f, -365.0f);
    m_MySceneObjects.push_back(drone);
    */

    //--------------------
    //　UI　小さい方のクロスヘア
    //--------------------
    m_bulletCrosshair = Game::GetInstance()->AddUI<Texture2D>();
    // Lv0は既存の白点画像。64pxの透明な余白の中央に小さい点がある。
    m_bulletCrosshair->SetTexture("assets/texture/PNG/White/crosshair001.png");
    m_bulletCrosshair->SetScale(BulletHoming::CrosshairSize(0), BulletHoming::CrosshairSize(0), 1.0f);
    m_mySceneObjects.push_back(m_bulletCrosshair);

    //--------------------
    //　UI　大きい方のクロスヘア
    //--------------------
    m_missileCrosshair = Game::GetInstance()->AddUI<Texture2D>();
    m_missileCrosshair->SetTexture("assets/texture/crosshair1310.png");
    m_missileCrosshair->SetScale(AimCursor::MissileDiameter, AimCursor::MissileDiameter, 10000.0f);
    m_mySceneObjects.push_back(m_missileCrosshair);

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
    m_mySceneObjects.push_back(explosionManager);
    m_mySceneObjects.push_back(Game::GetInstance()->AddObject<BulletManager>());
    m_mySceneObjects.push_back(Game::GetInstance()->AddObject<MissileManager>());
    m_mySceneObjects.push_back(Game::GetInstance()->AddObject<ParticleManager>());

    //--------------------
    //　UI　ミニマップ
    // --------------------
    CMiniMap* miniMap = Game::GetInstance()->AddUI<CMiniMap>();
    miniMap->SetPosition(-555.0f, 280.0f, 0.0f);
    miniMap->SetScale(150.0f, 150.0f, 1.0f);
    m_mySceneObjects.push_back(miniMap);

    //--------------------
    //　UI　タイマー
    // --------------------
    CTimer* timerUI = Game::GetInstance()->AddUI<CTimer>();
    timerUI->SetPosition(-50.0f, 320.0f, 0.0f); // 画面中央の上部
    timerUI->SetScale(40.0f, 60.0f, 1.0f); // 数字1桁あたりの大きさ
    m_mySceneObjects.push_back(timerUI);

    //--------------------
    //敵キャラのスポーン
    // --------------------
    CEnemySpawn* pawner = Game::GetInstance()->AddObject<CEnemySpawn>();
    m_mySceneObjects.push_back(pawner);

    //--------------------
    //砲台用のモデルの事前読み込み
    //--------------------
    m_mySceneObjects.push_back(Game::GetInstance()->AddObject<AutoTurretManager>());
}

//更新
void Stage1Scene::Update() {
    // カーソルはDraw側で更新し、一時停止中のDebug取得でもすぐに見た目を反映する。
}

void Stage1Scene::Draw(Camera* cam) {
    UpdateCrosshairs();
}

void Stage1Scene::UpdateCrosshairs() {
    const auto mouse = Input::GetMousePosition();
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    const int rank = players.empty() ? 0 : players[0]->GetBulletHomingRank();

    //マウス位置にクロスヘアを追従させる
    if (m_bulletCrosshair) {
        if ((rank > 0) != (m_bulletCrosshairRank > 0)) {
            m_bulletCrosshair->SetTexture(rank > 0 ? "assets/texture/crosshair061.png"
                                                   : "assets/texture/PNG/White/crosshair001.png");
        }
        m_bulletCrosshairRank = rank;
        const float size = BulletHoming::CrosshairSize(rank);
        m_bulletCrosshair->SetScale(size, size, 1.0f);
        m_bulletCrosshair->SetPosition(mouse.x, mouse.y, 0.0f);
    }
    if (m_missileCrosshair) {
        m_missileCrosshair->SetPosition(mouse.x, mouse.y, 0.0f);
    }
}

// 終了処理
void Stage1Scene::Uninit() {
    // 撃破済みの敵の生ポインタは参照せず、Gameが所有する生存中・追加待ちの物を解放する。
    Game::GetInstance()->DeleteAllObject();
    m_mySceneObjects.clear();
    CPresentBox::UnloadResource(); // 共有モデルの解放
    CSpanner::UnloadResource();
}
