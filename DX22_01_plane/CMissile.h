/// @file CMissile.h
/// @brief ミサイルの軌道追従、命中、爆発と画面揺れ。

#pragma once
/*
プレイヤーが右クリックで打つミサイル*/
#include "Object.h"
#include "TestModel.h"
#include "Bezier.h"
#include "billboard.h"
#include "PoolManager.h"

class CMissile : public Object {
private:
    /// ミサイルモデルの表示倍率。
    float m_modelScale = 5.5f;

    /// 機首の方向を求めるため、軌道の進行率を先読みする量。
    float m_directionLookAhead = 0.01f;

    /// 移動区間から衝突候補を絞るための距離。
    float m_collisionSearchRange = 100.0f;

    /// この距離以上離れた爆発ではカメラを揺らさない。
    float m_maxShakeDistance = 3000.0f;

    /// 至近距離で命中した場合の画面揺れの強さ。
    float m_maxShakePower = 10.0f;

    /// 命中時の画面揺れの持続時間（秒）。
    float m_shakeSeconds = 1.0f / 3.0f;

    /// 爆発ビルボードの表示サイズ。
    float m_explosionScale = 200.0f;

    /// 爆発の連番画像を1コマ表示する時間（秒）。
    float m_explosionFrameSeconds = 0.07f;

    /// キャラクターや弾の本体を描画するモデル。
    TestModel* m_body;
    /// ミサイルの飛行軌道と、その再生位置。
    Bezier m_bezier;
    /// ミサイルのダメージ量
    int m_damage = 3;

    /// ミサイルがアクティブかどうかのフラグ
    bool m_isActive = true;

    /// 当たり判定の球の半径（ワールド単位）。
    float m_colRadius = 2.0f;

    /// 追従・照準・攻撃の目標。
    Object* m_target = nullptr;

public:
    CMissile();
    ~CMissile();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    bool GetActive() {
        return m_isActive;
    }

    void SetActive(bool active) {
        m_isActive = active;
    }

    //誰から誰に
    void Shoot(Object& shooter, Object& target, float angleOffsetDebug);

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_colRadius};
    }

    void SetColRadius(float radius) {
        m_colRadius = radius;
    }

    // プール再利用時も、発射したプレイヤーの現在の強化を反映する。
    void SetDamage(int value) {
        m_damage = value;
    }
};

using MissileManager = PoolManager<CMissile, 25>; // 最大25発を再利用する。20発の一斉発射にも対応。
using ExplosinManager = PoolManager<billboard, 16>;
