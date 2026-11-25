/// @file CMissile.h
/// @brief ミサイルの軌道追従、命中、爆発と画面揺れ。

#pragma once
#include "Object.h"
#include "TestModel.h"
#include "Bezier.h"
#include "billboard.h"
#include "PoolManager.h"
#include "SurvivorProgression.h"
#include <memory>

class CMissile : public Object {
private:
    /// ミサイルモデルの各軸にかける描画倍率。
    static constexpr DirectX::SimpleMath::Vector3 ModelScale{5.5f};
    /// 機首の向きを求めるために先読みする軌道の割合（全行程を1とする）。
    static constexpr float DirectionLookAheadFraction = 0.01f;
    /// 爆発による画面揺れが届く最大距離（ワールド単位）。
    static constexpr float MaxShakeDistance = 3000.0f;
    /// プレイヤーの位置で命中した場合の画面揺れの強さ。
    static constexpr float MaxShakePower = 3.0f;
    /// 命中時の画面揺れの持続時間（秒）。
    static constexpr float ShakeSeconds = 0.15f;
    /// 爆発の連番画像を1コマ表示する時間（秒）。
    static constexpr float ExplosionFrameSeconds = 0.07f;

    /// プールの全ミサイルが同じモデルを所有し、描画時には各ミサイルの位置・回転を渡す。
    std::shared_ptr<TestModel> m_sharedModel;
    /// このミサイルのワールド空間の飛行軌道と進行率。発射のたびに作り直す。
    Bezier m_bezier;
    /// 命中時に敵へ与えるダメージ。発射時の武器レベルで決まる。
    int m_damage = 3;
    /// 発射時に決まる爆風半径と爆発画像の倍率。
    float m_blastScale = 1.0f;
    /// 当たり判定の球の半径（ワールド単位）。
    float m_colRadius = 2.0f;
    /// 今回の発射で追尾する敵への非所有参照。敵リストで生存を確かめてから使う。
    Object* m_target = nullptr;

public:
    /// 外周で敵に渡す、端数を切り上げた半分のダメージ。
    static constexpr int OuterBlastDamage(int damage) {
        return damage / 2 + damage % 2;
    }

    /// 爆心から敵の球までの距離でダメージを決め、必要なら全ダメージ帯への命中も返す。
    static int BlastDamage(int damage, const DirectX::SimpleMath::Vector3& center,
                           const Collision::Sphere& target, float blastScale = 1.0f,
                           bool* fullHit = nullptr) {
        const float squaredDistance = (center - target.center).LengthSquared();
        const float fullRadius = UpgradeBalance::BaseMissileFullDamageRadius * blastScale + target.radius;
        const float outerRadius = UpgradeBalance::BaseMissileBlastRadius * blastScale + target.radius;
        const bool full = damage > 0 && squaredDistance <= fullRadius * fullRadius;
        if (fullHit) {
            *fullHit = full;
        }
        if (damage <= 0 || squaredDistance > outerRadius * outerRadius) {
            return 0;
        }
        return full ? damage : OuterBlastDamage(damage);
    }

    /// プールの初期化時にミサイル用の共有モデルを取得する。
    void Init() override;
    /// 飛行軌道と命中判定を進め、命中または軌道終了でプールに戻す。
    void Update() override;
    /// このミサイルのワールド位置と回転で共有モデルを描画する。
    void Draw(Camera* cam) override;
    /// このミサイルが持つモデル参照を解放する。
    void Uninit() override;

    /// 指定した水平オフセット角（度）で発射し、この弾の爆風拡張レベルを確定する。
    void Shoot(Object& shooter, Object& target, float angleOffsetDegrees, int blastRank = 0);

    /// HUDが生存中の敵と照合する。プールへ戻ったミサイルには追尾先がない。
    const Object* GetTrackingTarget() const {
        return GetActive() ? m_target : nullptr;
    }

    /// 現在のワールド位置を中心とする当たり判定の球を返す。
    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_colRadius};
    }

    /// 当たり判定の半径（ワールド単位）を変更する。
    void SetColRadius(float radius) {
        m_colRadius = radius;
    }

    /// 今回の発射に使うダメージを設定する。
    void SetDamage(int value) {
        m_damage = value;
    }
};

using MissileManager = PoolManager<CMissile, 25>; // 最大25発を再利用する。20発の一斉発射にも対応。
using ExplosinManager = PoolManager<billboard, 16>;
