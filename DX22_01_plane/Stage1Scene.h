#pragma once
#include "Scene.h"
#include "Object.h"
#include"Texture2D.h"
// Stage1Scenクラス
class Stage1Scene : public Scene
{
private:
	std::vector<Object*> m_MySceneObjects; // このシーンのオブジェクト

    Texture2D* m_Crosshair = nullptr; // クロスヘアのテクスチャ

	void Init(); // 初期化
	void Uninit(); // 終了処理

	

public:
	Stage1Scene(); // コンストラクタ
	~Stage1Scene(); // デストラクタ
	void Update(); // 更新

	

};

