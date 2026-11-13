/// @file CPlayer.h
/// @brief プレイヤーの移動、照準、射撃、ロボットの姿勢更新。

#pragma once
/*
プレイヤークラス
*/
#include "Object.h"
#include "TestCube.h"
#include "GolfBall.h"
#include "Bezier.h"
#include <iostream>
#include <vector>
#include "COrbitalOrb.h"
#include "SurvivorProgression.h"
#include "PlayerRobotModel.h"
#include "CEmitter.h"
#include <string>

class CPlayer : public Object {
private:
    /// 射撃クリックで報酬を即決しないようにする入力待ち時間（秒）。
    float m_selectionInputDelay = 0.2f;
    /// 強化を取得した通知の表示時間（秒）。
    float m_rewardNoticeDuration = 4.0f;
    /// 被弾後に接触ダメージの連続発生を防ぐ時間（秒）。
    float m_damageInvincibilitySeconds = 1.0f / 3.0f;

    /// 重力加速度（ワールド単位/秒²）。
    float m_gravity = 28.8f;

    /// 移動速度強化を掛ける前の歩行速度（ワールド単位/秒）。
    float m_moveSpeed = 60.0f;

    /// ジャンプ開始時の上向き速度（ワールド単位/秒）。
    float m_jumpSpeed = 30.0f;

    /// プレイヤー本体の当たり判定の半径。
    float m_collisionRadius = 10.0f;

    /// 判定中心から弾の発射位置までの高さ。
    float m_muzzleHeight = 5.0f;

    /// 追加の通常弾を左右へ広げる角度（ラジアン）。
    float m_multiShotAngle = 0.065f;

    /// 守護オーブがプレイヤーの周囲を回る半径。
    float m_orbitRadius = UpgradeBalance::OrbitRadius;

    /// 守護オーブの回転速度（ラジアン/秒）。
    float m_orbitAngularSpeed = 3.0f;

    /// 現在のミサイルの最大数
    int m_currentRocketCount = 1;
    /// 最大ミサイル数
    const int m_maxRocketCount = 20;
    /// 骨格アニメーション付きの6本脚ロボット。
    PlayerRobotModel* m_body;
    /// 開始時の脚の向き。カメラ旋回では変えない。
    float m_lowerBodyYaw = 0.0f;
    /// カーソルの照準へ向ける仰角。上向きが正（ラジアン）。
    float m_upperBodyPitch = 0.0f;

    // 発射間隔と残り待ち時間は秒。実測したdeltaSecondsを引いて更新する。
    /// 通常弾の発射間隔（秒）。
    float m_bulletTime = UpgradeBalance::BulletInterval;
    /// 0以下なら発射可能。小数部分も保持する。
    float m_currentBulletTime = 0.0f;
    /// 通常弾が実際に発射されたとき、発射位置へ火花を放出する。
    CEmitter m_bulletEmitter;

    // ミサイルは初期状態で2秒おきに発射できる。
    /// ミサイルの発射間隔（秒）。
    float m_missileTime = UpgradeBalance::MissileInterval;
    /// ミサイルを再発射できるまでの残り秒数。
    float m_currentMissileTime = 0.0f;

    /// 位置を変化させる速度ベクトル（ワールド単位/秒）。
    DirectX::SimpleMath::Vector3 m_velocity = {0, 0, 0};
    /// 地面についているか
    bool m_isGrounded = false;

    /// 現在の体力。0以下になると死亡処理を行う。
    int m_hp = UpgradeBalance::BaseMaxHp;

    /// オーブのリスト
    std::vector<COrbitalOrb*> m_orbs;

    // 成長状態はプレイヤーごとに保持し、新しいゲームではLv1から始める。
    /// このプレイヤーのレベル・経験値・所持スキル・選択待ち報酬。
    SurvivorProgression m_progression;
    /// このプレイヤーが倒した敵の累計数。
    int m_kills = 0;

    // 自動武器・回復・無敵時間の残り秒数。ゲーム停止中は進めない。
    /// 自動回復が次に発動するまでの残り秒数。
    float m_regenerationTimer = 0.0f;
    /// 衝撃波が次に発動するまでの残り秒数。
    float m_shockwaveTimer = 0.0f;
    /// 連鎖電撃が次に発動するまでの残り秒数。
    float m_lightningTimer = 0.0f;
    /// 被弾後の無敵時間の残り秒数。
    float m_damageCooldown = 0.0f;

    // 選択直前の射撃入力が、報酬の選択へそのまま使われることを防ぐ待ち時間。
    /// 射撃入力が報酬選択へ持ち越されるのを防ぐ待ち時間（秒）。
    float m_selectionDelay = 0.0f;
    /// 詳細を読んでいる間はレベルアップと同様にゲームを停止。
    bool m_showLoadout = false;

    // 通知だけは描画の経過時間を使うため、こちらの単位は秒。
    /// 獲得した報酬の通知を表示する残り秒数。
    float m_rewardNoticeTime = 0.0f;
    /// 画面へ表示する報酬の通知文。
    std::string m_rewardNotice;

#ifdef _DEBUG
    // プレイヤー作成時に初期化し、Releaseにはデバッグ用状態を持ち込まない。
    /// デバッグ用にゲーム進行を一時停止しているか。
    bool m_debugPaused = false;
    /// デバッグ用の無敵状態が有効か。
    bool m_debugInvincible = false;
    /// デバッグ操作で一度に追加する経験値。
    int m_debugExperience = 5;
    /// 0=入力に合わせる、1以降=その場でアニメーションを確認。
    int m_debugRobotMotion = 0;
#endif

    // 実装はSurvivorCombat.cpp。報酬の確定と、実際の戦闘への反映を分けている。
    void ApplyUpgradeEffects(UpgradeId id, bool fromPresent);
    void UpdateAutoWeapons();
    void DeployTurret();

    // 表示と通常弾が、同じカーソルの照準位置・発射基準位置を使う。
    bool TryGetAimTarget(DirectX::SimpleMath::Vector3& targetPosition) const;
    DirectX::SimpleMath::Vector3 GetBulletOrigin() const;
    void UpdateAim();

public:
    CPlayer();
    ~CPlayer();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    //WASD、SPACEの挙動
    void Move();
    //床との当たり判定
    void isGrounded();
    //左クリック時にBulletを作り
    void StartBullet();

    void StartMissile();

    DirectX::SimpleMath::Vector3 GetVelocity() {
        return m_velocity;
    }

    void SetVelocity(float y) {
        m_velocity.y = y;
    }

    //void SetMaterial(DirectX::SimpleMath::Color col);

    //着地中の処理.
    void Onland();

    //当たり判定用のBOXを取得
    DirectX::SimpleMath::Vector3 GetPosition() const {
        return m_position;
    }

    DirectX::SimpleMath::Vector3 GetScale() const {
        return m_scale;
    }

    // 当たり判定用（車体の位置を返す）
    //DirectX::SimpleMath::Vector3 GetPosition() { return m_Position; }

    int GetHp() {
        return m_hp;
    }

    int GetMaxHp() {
        return UpgradeBalance::BaseMaxHp +
               UpgradeBalance::VitalityPerRank * m_progression.Rank(UpgradeId::Vitality);
    }

    int GetLevel() const {
        return m_progression.Level();
    }

    int GetKills() const {
        return m_kills;
    }

    // 照準の表示と飛翔中の弾で、同じ現在レベルを参照する。
    int GetBulletHomingRank() const {
        return m_progression.Rank(UpgradeId::BulletHoming);
    }

    // Game::UpdateはIsGameplayPausedを見てゲーム進行を止める。Debug停止中の自由視点は別更新。
    bool IsChoosingUpgrade() const {
        return m_progression.IsChoosing();
    }

    bool IsGameplayPaused() const {
#ifdef _DEBUG
        if (m_debugPaused) {
            return true;
        }
#endif
        return IsChoosingUpgrade() || m_showLoadout;
    }

    // 撃破報酬、プレゼント、画面表示から呼ばれる成長システムの入口。
    void OnEnemyDefeated(int experience);
    void GivePresentReward();
    void DrawSurvivorUI();
    DirectX::SimpleMath::Vector3 GetTurretFollowPosition() const;
#ifdef _DEBUG
    void DrawSurvivorDebugUI();

    bool IsDebugPaused() const {
        return m_debugPaused;
    }

    bool DebugGiveUpgrade(UpgradeId id);
#endif
    void Heal(int amount);

    float GetMissileTime() {
        return m_missileTime;
    }

    float GetCurrentMissileTime() {
        return m_currentMissileTime;
    }

    void CreateOrbs(int count);

    //debug用
    int getCurRocket() {
        return m_currentRocketCount;
    }

    void SetCurRocket(int num) {
        //debug用にミサイルの最大数を設定する
        if (num < 1) {
            num = 1;
        }
        if (num > m_maxRocketCount) {
            num = m_maxRocketCount;
        }
        m_currentRocketCount = num;
    }

    void SetHP(int num) {
        m_hp = num;
    }

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius}; //半径10
    }

    void TakeDamage(int damage);
};
