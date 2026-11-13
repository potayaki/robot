/// @file Scene.h
/// @brief ゲームシーンの初期化・更新・描画・終了の共通インターフェース。

#pragma once
#include <memory>
#include "Camera.h"

//Scene クラス
class Scene {
public:
    Scene(); // コンストラクタ
    virtual ~Scene(); // デストラクタ(virtualが必要)

    /// 経過時間と入力に合わせて状態を更新する。
    virtual void Update() = 0; // 更新（純粋仮想関数）
    /// 現在の状態を画面へ描画する。
    virtual void Draw(Camera* cam){};

    /// ImGuiのフレーム開始後に画面のボタン・結果を描画する。
    virtual void DrawUI() {}
};
