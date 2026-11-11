/// @file ResultScene.h
/// @brief ゲーム終了後の結果画面。

#pragma once
#include "Scene.h"
#include "Object.h"

// ResultSceneクラス
class ResultScene : public Scene {
private:
    /// このシーンのオブジェクト
    std::vector<Object*> m_mySceneObjects;

    /// 使用前の状態と必要な資源を初期化する。
    void Init(); // 初期化
    /// 使用した資源を解放して終了する。
    void Uninit(); // 終了処理

public:
    ResultScene(); // コンストラクタ
    ~ResultScene(); // デストラクタ

    void Setscore(int c); //スコアを設定

    /// 経過時間と入力に合わせて状態を更新する。
    void Update(); // 更新
};
