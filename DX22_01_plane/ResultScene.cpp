#include "ResultScene.h"
#include "Game.h"
#include "Input.h"
#include"Texture2D.h"
#include"TitleScene.h"
// コンストラクタ
ResultScene::ResultScene()
{
	Init();

}

// デストラクタ
ResultScene::~ResultScene()
{
	Uninit();
}


// 初期化
void ResultScene::Init()
{
//Renderer::SetClearColor(0.2f, 0.2f, 0.2f);

}

// 更新
void ResultScene::Update()
{
	/*
	Scene::Update();

	// エンターキーを押してタイトルへ
	if (Input::GetKeyTrigger(VK_SPACE))
	{
		//Game::GetInstance()->ChangeScene<TitleScene>();
		//Game::GetInstance()->ChangeScene<TitleScene>();
	}
	*/

}

// 終了処理
void ResultScene::Uninit()
{
	
}

void ResultScene::Setscore(int c) {
	Texture2D* stringObj = dynamic_cast<Texture2D*>(m_MySceneObjects[1]);
	switch (c) {
		case -4:
		stringObj->SetUV(1, 2, 1, 13);//-4コンドル
		break;
		case -3:
		stringObj->SetUV(1, 3, 1, 13);//-3アルバトロス
		break;
		case -2:
		stringObj->SetUV(1, 4, 1, 13);//-2イーグル
		break;
		case -1:
		stringObj->SetUV(1, 5, 1, 13);//-1バーディ
		break;
		case 0:
		stringObj->SetUV(1, 6, 1, 13);//Par
		break;
		case 1:
		stringObj->SetUV(1, 7, 1, 13);//+1ボギー
		break;
		case 2:
		stringObj->SetUV(1, 8, 1, 13);//+2ダブルボギー
		break;
		case 3:
		stringObj->SetUV(1, 9, 1, 13);//+3トリプルボギー
		break;
		case 4:
		stringObj->SetUV(1, 10, 1, 13);//+4
		break;
		case 5:
		stringObj->SetUV(1, 12, 1, 13);//+5
		break;
		case 6:
		stringObj->SetUV(1, 13, 1, 13);//+6
		break;
		default:

		break;
	}
}