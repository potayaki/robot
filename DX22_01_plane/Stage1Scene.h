/// @file Stage1Scene.h
/// @brief ステージの地形、敵、UI、照準画像の初期配置。

#pragma once
#include "Scene.h"
#include "Object.h"
#include "Texture2D.h"

// Stage1Scenクラス
class Stage1Scene : public Scene {
private:
    /// このシーンのオブジェクト
    std::vector<Object*> m_mySceneObjects;

    /// 点と枠の切替時だけ画像を読み直す。
    int m_bulletCrosshairRank = 0;
    void UpdateCrosshairs(); // クロスヘアの更新

    /// 弾のクロスヘアのテクスチャ
    Texture2D* m_bulletCrosshair = nullptr;
    /// ミサイルのクロスヘアのテクスチャ
    Texture2D* m_missileCrosshair = nullptr;

    /// 使用前の状態と必要な資源を初期化する。
    void Init(); // 初期化
    /// 使用した資源を解放して終了する。
    void Uninit(); // 終了処理

public:
    Stage1Scene(); // コンストラクタ
    ~Stage1Scene(); // デストラクタ
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(); // 更新
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
};
