#pragma once
#include <iostream>
#include"TestCube.h"
#include "TestPlane.h"
#include"TestModel.h"

#include"Renderer.h"
#include"TitleScene.h"
#include"Stage1Scene.h"
#include"ResultScene.h"

enum SceneName {
	TITLE,
	STAGE1,
	RESULT
};

class Game
{
private:
	static Game* m_instance;//ゲームインスタンス
	Scene* m_scene;//シーン
	// カメラ
	Camera  m_Camera;

	// テストオブジェクト
	TestPlane m_Plane;
	
	TestModel m_model;
	
	std::vector<std::unique_ptr<Object>> m_objects;

public:
	Game(); // コンストラクタ
	~Game(); // デストラクタ

	static void Init(); // 初期化
	static void Update(); // 更新
	static void Draw(); // 描画
	static void Uninit(); // 終了処理

	static Game* GetInstance();

	Camera* GetCamera() { return &m_Camera; }

	void ChangeScene(SceneName sName);//シーンの変更
	void DeleteObject(Object* pt);//オブジェクトを削除
	void DeleteAllObject();//オブジェクトをすべて削除

	//オブジェクトを追加する(テンプレート関数なのでここに直接記述)
	template<typename T>T* AddObject() {	
		T* pt = new T;
		m_instance->m_objects.emplace_back(pt);
		pt->Init();//初期化
		return pt;
	}

	//オブジェクトを追加する
template<typename T>std::vector<T*>GetObjects() {
	std::vector<T*> res;
	for (auto& a : m_instance->m_objects) {
		//dybamiccastで型をチェック
		if (T* derivedObj = dynamic_cast<T*>(a.get())) {
			res.emplace_back(derivedObj);
		}
	}
	return res;
}
};