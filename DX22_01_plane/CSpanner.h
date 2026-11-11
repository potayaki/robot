/// @file CSpanner.h
/// @brief 回復スパナの出現、浮遊表示、取得時のHP回復。

#pragma once
#include "TestModel.h"

// Enemy drop: repairs the robot when collected.
class CSpanner : public Object {
public:
    // Drop chance for every enemy defeat, in percent. 0 = never, 100 = always.
    /// 敵撃破時に回復スパナが出現する確率（パーセント）。
    inline static int m_spawnChancePercent = 15;
    /// 拾ったときに最大HPの何パーセントを回復するか。
    int m_healPercent = 25;
    /// 回復スパナの基本倍率。脈動による拡縮を掛ける前の大きさ。
    float m_baseScale = 5.0f;
    /// 回復アイテムの脈動による拡縮の振幅。
    float m_pulseAmount = 0.20f;
    /// 回復スパナの回転速度（ラジアン/秒）。
    float m_rotationSpeed = 2.0f;

    ~CSpanner() override {
        Uninit();
    }

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
    static void LoadResource();
    static void UnloadResource();

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_pickupRadius};
    }

private:
    /// 浮遊と拡大縮小を一周させる時間（秒）。
    float m_bobPeriod = 1.2f;

    /// 取得判定の位置から浮かせる標準の高さ。
    float m_hoverHeight = 10.0f;

    /// 浮遊中の高さの振れ幅。
    float m_bobAmplitude = 2.0f;

    /// 同種のアイテム間で共有する読込済みモデル。
    inline static std::unique_ptr<TestModel> m_sharedModel;
    /// 共有資源を使用しているインスタンス数。最後の終了時に資源を解放する。
    inline static int m_instanceCount = 0;
    /// 同時に存在できる回復アイテムの上限数。
    int m_maxCount = 15;
    /// プレイヤーがアイテムを拾える判定半径。
    float m_pickupRadius = 14.0f;
    /// 現在の動作または演出を開始してからの経過秒数。
    float m_elapsed = 0;
    /// このインスタンスを共有モデルの使用数へ加算済みか。
    bool m_counted = false;
};
