#pragma once
#include "Scene.h"
#include "Object.h"
#include"Texture2D.h"
// Stage1Scenクラス
class Stage1Scene : public Scene
{
private:
	std::vector<Object*> m_MySceneObjects; // このシーンのオブジェクト

    int m_BulletCrosshairRank = 0; // 点と枠の切替時だけ画像を読み直す。
    void UpdateCrosshairs();// クロスヘアの更新

    
    Texture2D* m_BulletCrosshair = nullptr; // 弾のクロスヘアのテクスチャ
    Texture2D* m_MissileCrosshair = nullptr; // ミサイルのクロスヘアのテクスチャ
    

	void Init(); // 初期化
	void Uninit(); // 終了処理

	

public:
	Stage1Scene(); // コンストラクタ
	~Stage1Scene(); // デストラクタ
	void Update(); // 更新
    void Draw(Camera* cam) override;

	

};

