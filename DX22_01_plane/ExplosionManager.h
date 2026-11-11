/// @file ExplosionManager.h
/// @brief 再利用可能な爆発エフェクトの管理。

#pragma once
#include "Object.h"
#include "billboard.h"

class ExplosionManager : public Object {
private:
    /// 画面に出せる最大爆発数
    static const int POOL_SIZE = 20;
    /// 再利用するオブジェクトを保持する一覧。
    billboard* m_pool[POOL_SIZE];
    /// 更新・描画の対象として有効になっているか。
    bool m_active[POOL_SIZE];

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // 外部から爆発を発生させる関数
    void CreateExplosion(DirectX::SimpleMath::Vector3 pos);
};
