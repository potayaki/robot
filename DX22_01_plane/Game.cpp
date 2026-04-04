#include "Game.h"
#include "Renderer.h"
#include"input.h"


Game* Game::m_instance;//ゲームインスタンス

// コンストラクタ
Game::Game()
{
	m_scene = nullptr;
}

// デストラクタ
Game::~Game()
{
	delete m_scene;
	DeleteAllObject();//すべてのオブジェクトを削除
}

// 初期化
void Game::Init()
{
	//インスタンス作成
	m_instance = new Game;
	// 描画初期化処理
	Renderer::Init();
	Input::Create();

	// カメラ初期化
	m_instance->m_Camera.Init();
	//最初のシーンを読み込む
	m_instance->m_scene = new TitleScene;
	
	
}

// 更新
void Game::Update()
{
	//シーン更新
	Input::Update();
	//シーン更新
	m_instance->m_scene->Update();
	// カメラ更新
	m_instance->m_Camera.Update();
	
	// テストオブジェクト更新
	for (auto& a : m_instance->m_objects) {
		a->Update();
	}
}

// 描画
void Game::Draw()
{
	// 描画前処理
	Renderer::DrawStart();

	// テストオブジェクト描画
	for (auto& a : m_instance->m_objects) {
		a->Draw(&m_instance->m_Camera);
	}
	if (m_instance->m_scene) {
		m_instance->m_scene->Draw(&m_instance->m_Camera);
	}
	// 描画後処理
	Renderer::DrawEnd();
}

// 終了処理
void Game::Uninit()
{
	// カメラ終了処理
	m_instance->m_Camera.Uninit();

	// テストオブジェクト終了処理
	m_instance->m_Plane.Uninit();
	m_instance->m_model.Uninit();
	for (auto& a : m_instance->m_objects) {
		a->Uninit();
}
	Input::Release();
	// 描画終了処理
	Renderer::Uninit();

	delete m_instance;
}

Game* Game::GetInstance() {
	return m_instance;
}

//シーンを切り替える
void Game::ChangeScene(SceneName sName) {
	//読み込み済みのシーンがあれば削除
	int score = 0;
	
	if (m_instance->m_scene != nullptr) {
		//消そうとしているシーンがStage1ならスコアを保存しておく
		/*
		if (Stage1Scene* sObj = dynamic_cast<Stage1Scene*>(m_instance->m_scene)) {
			score = sObj->Getscore();
		}
		*/
		delete m_instance->m_scene;
		m_instance->m_scene = nullptr;
	}
	

	switch (sName) {
		case TITLE:
		m_instance->m_scene = new TitleScene;
		break;
		case STAGE1:
		m_instance->m_scene = new Stage1Scene;
		break;
		case RESULT:
		m_instance->m_scene = new ResultScene;
		dynamic_cast<ResultScene*>(m_instance->m_scene)->Setscore(score);//スコアを設定
		break;
	}
}

void Game::DeleteObject(Object* pt) {
	if (pt == NULL) {
		return;
	}
	pt->Uninit();

	//要素を削除
	erase_if(m_instance->m_objects, [pt](const std::unique_ptr<Object>& element) {
		return element.get() == pt;
	});
	m_instance->m_objects.shrink_to_fit();
}

void Game::DeleteAllObject() {

	for (auto& o : m_instance->m_objects) {
		o->Uninit();
	}
	m_instance->m_objects.clear();
	m_instance->m_objects.shrink_to_fit();
}