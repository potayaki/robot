#pragma once
#include <iostream>
#include<memory>
#include"Object.h"
#include"TestCube.h"
#ifdef _DEBUG
#include "CollisionDebugDraw.h"
#endif
#include "TestPlane.h"
#include"TestModel.h"
#include"Renderer.h"
#include "SkyEnvironment.h"
#include"TitleScene.h"
#include"Stage1Scene.h"
#include"ResultScene.h"
#include"TestCube.h"
enum SceneName {
	TITLE,
	STAGE1,
	RESULT
};

class Game
{
private:
    static void FlushObjects(); // 更新終了後に、削除・追加待ちのオブジェクトを反映する。
	static Game* m_instance;//ゲームインスタンス
	Scene* m_scene;//シーン
	// カメラ
	Camera  m_Camera;

    SkyEnvironment m_sky; // 昼夜・天候と、それに合わせた空・照明。

	// テストオブジェクト
	TestPlane m_Plane;
	
	TestModel m_model;
	
	std::vector<std::unique_ptr<Object>> m_objects; // オブジェクトのリスト
    std::vector<std::unique_ptr<Object> >m_addObjects;    // 追加待ちリスト

    std::vector<std::unique_ptr<Object>> m_UIs; // オブジェクトのリスト
    std::vector<std::unique_ptr<Object> >m_addUIs;    // 追加待ちリスト

#ifdef _DEBUG
    CollisionDebugDraw m_CollisionDebug;
    bool m_ShowCollision = false;
#endif



public:
	Game(); // コンストラクタ
	~Game(); // デストラクタ

    double m_gameTime = 0.0; // 一時停止を除いた実測の経過秒数。

	static void Init(); // 初期化
	static void Update(double elapsedSeconds); // 更新
	static void Draw(); // 描画
	static void Uninit(); // 終了処理

	static Game* GetInstance();

	Camera* GetCamera() { return &m_Camera; }

	void ChangeScene(SceneName sName);//シーンの変更

	void DeleteAllObject();//オブジェクトをすべて削除

	//オブジェクトを追加する(テンプレート関数
	template<typename T>T* AddObject() {	
		T* pt = new T;
		m_instance->m_addObjects.emplace_back(pt);
		pt->Init();//初期化
		return pt;
	}

    template<typename T>T* AddUI() {
        T* pt = new T;
        m_instance->m_addUIs.emplace_back(pt);
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

Scene* GetScene() { return m_scene; }

};
