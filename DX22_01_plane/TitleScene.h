/// @file TitleScene.h
/// @brief タイトル画面とステージ開始への切り替え。

#pragma once
#include "Scene.h"
#include "Object.h"

// TitleSceneクラス
class TitleScene : public Scene {
private:
    /// このシーンのオブジェクト
    std::vector<Object*> m_mySceneObjects;

    /// 使用前の状態と必要な資源を初期化する。
    void Init(); // 初期化
    /// 使用した資源を解放して終了する。
    void Uninit(); // 終了処理

public:
    TitleScene(); // コンストラクタ
    ~TitleScene(); // デストラクタ

    /// 経過時間と入力に合わせて状態を更新する。
    void Update(); // 更新
};
