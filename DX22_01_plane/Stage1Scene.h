/// @file Stage1Scene.h
/// @brief ステージの地形、敵、UI、照準画像の初期配置。

#pragma once
#include "Scene.h"
#include "Object.h"
#include "Texture.h"
#include "Texture2D.h"

class CPlayer;

// Stage1Scenクラス
class Stage1Scene : public Scene {
private:
    /// このシーンのオブジェクト
    std::vector<Object*> m_mySceneObjects;

    /// 点と枠の切替時だけ画像を読み直す。
    int m_bulletCrosshairRank = 0;
    /// 現在の通常弾レベルとマウス位置に合わせ、大小の照準を更新する。
    void UpdateCrosshairs(int rank);
    /// 次の発射先と追尾中の敵の中心にロックオンマーカーを描画する。
    void DrawMissileLocks(const Camera& camera, const CPlayer& player) const;
    /// 敵に重ねるマーカーの中央画像。全対象で共有する。
    Texture m_lockOnTexture;

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
