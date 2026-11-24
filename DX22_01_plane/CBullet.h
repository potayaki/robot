/// @file CBullet.h
/// @brief 通常弾の発射、移動、誘導、移動区間での命中判定。

#pragma once
#include "TestModel.h"
#include "Object.h"
#include "PoolManager.h"
#include <memory>
using namespace DirectX::SimpleMath;

class CBullet : public Object {
private:
    /// 1発が飛行できる時間（秒）。発射時に残り寿命へコピーする。
    static constexpr float LifetimeSeconds = 2.0f;
    /// 発射速度（ワールド単位/秒）。
    static constexpr float LaunchSpeed = 1500.0f;
    /// 誘導した軌道を折れ線で残す間隔（ゲーム内の秒）。通常弾速で37.5ワールド単位。
    static constexpr float TrailIntervalSeconds = 1.0f / 40.0f;
    /// 通過した線分の残光が消えるまでのゲーム内秒数。
    static constexpr float TrailLifetimeSeconds = 0.09f;
    /// 弾モデルのローカルX・Y・Z軸にかける描画倍率。
    static constexpr Vector3 ModelScale{0.8f, 4.0f, 0.8f};

    /// プールの全通常弾が同じモデルを所有し、描画時には各弾の位置・回転を渡す。
    std::shared_ptr<TestModel> m_sharedModel;
    /// この弾の現在の移動速度。誘導で向きが変わる（ワールド単位/秒）。
    Vector3 m_velocity;
    /// 今回の発射から消滅までの残り時間（秒）。
    float m_remainingLifeSeconds = 0.0f;
    /// 前回の軌跡発生から繰り越した時間（秒）。発射時にリセットする。
    float m_trailElapsedSeconds = 0.0f;
    /// 確定した残光の終点。次の線分の始点として発射ごとに銃口へ戻す。
    Vector3 m_trailStart;
    /// 命中時に敵へ与えるダメージ。発射時の武器レベルで決まる。
    int m_damage = 1;
    /// 飛行中もプレイヤーの強化に追従する誘導レベル。0なら直進。
    int m_homingRank = 0;
    /// 当たり判定の球の半径（ワールド単位）。
    float m_colRadius = 2.0f;

public:
    /// プールの初期化時に通常弾用の共有モデルを取得する。
    void Init() override;
    /// 誘導・移動・命中判定を進め、命中または寿命切れでプールに戻す。
    void Update() override;
    /// この弾のワールド位置と回転で共有モデルを描画する。
    void Draw(Camera* cam) override;
    /// 不透明物体の描画後に、直前の残光の終点から飛行中の弾までをつなぐ。
    void DrawTrailTip(Camera* cam) const;
    /// この弾が持つモデル参照を解放する。
    void Uninit() override;

    /// 現在の速度ベクトル（ワールド単位/秒）を設定する。
    void SetVelocity(Vector3 velocity) {
        m_velocity = velocity;
    }

    /// 現在の速度ベクトル（ワールド単位/秒）を返す。
    Vector3 GetVelocity() const {
        return m_velocity;
    }

    /// プールから取得した弾を銃口のワールド座標から指定方向へ発射する。
    void Shoot(Vector3 muzzlePosition, Vector3 direction, int homingRank = 0);

    /// 弾のワールド位置を中心とする当たり判定の球を返す。
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

using BulletManager = PoolManager<CBullet, 96>; // マルチショットと高速装填の最大強化にも対応。
