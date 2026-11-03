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
#include <string>

class CPlayer : public Object
{
private:
    int curRocket = 1; //現在のミサイルの最大数
    const int maxRocket = 20; //最大ミサイル数
    PlayerRobotModel* m_body; // 骨格アニメーション付きの6本脚ロボット。
    float m_lowerBodyYaw = 0.0f; // 開始時の脚の向き。カメラ旋回では変えない。
    float m_upperBodyPitch = 0.0f; // カーソルの照準へ向ける仰角。上向きが正（ラジアン）。

    // 発射間隔と残り待ち時間は秒。実測したdeltaSecondsを引いて更新する。
    float m_BulletTime = 0.1f;
    float m_currentBulletTime = 0.0f; // 0以下なら発射可能。小数部分も保持する。

    // ミサイルは初期状態で2秒おきに発射できる。
    float m_MissileTime = 2.0f;
    float m_currentMissileTime = 0.0f;

    DirectX::SimpleMath::Vector3 m_velocity = {0, 0, 0};
    bool IsGrounded = false; //地面についているか

    int m_hp = 100;

    std::vector<COrbitalOrb*> m_orbs; // オーブのリスト

    // 成長状態はプレイヤーごとに保持し、新しいゲームではLv1から始める。
    SurvivorProgression m_progression;
    int m_kills = 0;

    // 自動武器・回復・無敵時間の残り秒数。ゲーム停止中は進めない。
    float m_regenerationTimer = 0.0f;
    float m_shockwaveTimer = 0.0f;
    float m_lightningTimer = 0.0f;
    float m_damageCooldown = 0.0f;

    // 選択直前の射撃入力が、報酬の選択へそのまま使われることを防ぐ待ち時間。
    float m_selectionDelay = 0.0f;
    bool m_showLoadout = false; // 詳細を読んでいる間はレベルアップと同様にゲームを停止。

    // 通知だけは描画の経過時間を使うため、こちらの単位は秒。
    float m_rewardNoticeTime = 0.0f;
    std::string m_rewardNotice;

#ifdef _DEBUG
    // プレイヤー作成時に初期化し、Releaseにはデバッグ用状態を持ち込まない。
    bool m_debugPaused = false;
    bool m_debugInvincible = false;
    int m_debugExperience = 5;
    int m_debugRobotMotion = 0; // 0=入力に合わせる、1以降=その場でアニメーションを確認。
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

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    //WASD、SPACEの挙動
    void Move();
    //床との当たり判定
    void isGrounded();
    //左クリック時にBulletを作り
    void StartBullet();

    void StartMissile();

    DirectX::SimpleMath::Vector3 GetVelocity()
    {
        return m_velocity;
    }

    void SetVelocity(float y)
    {
        m_velocity.y = y;
    }

    //void SetMaterial(DirectX::SimpleMath::Color col);

    //着地中の処理.
    void Onland();

    //当たり判定用のBOXを取得
    DirectX::SimpleMath::Vector3 GetPosition() const
    {
        return m_Position;
    }

    DirectX::SimpleMath::Vector3 GetScale() const
    {
        return m_Scale;
    }

    // 当たり判定用（車体の位置を返す）
    //DirectX::SimpleMath::Vector3 GetPosition() { return m_Position; }

    int GetHp()
    {
        return m_hp;
    }

    int GetMaxHp()
    {
        return 100 + 20 * m_progression.Rank(UpgradeId::Vitality);
    }

    int GetKills() const
    {
        return m_kills;
    }

    // Game::UpdateはIsGameplayPausedを見てゲーム進行を止める。Debug停止中の自由視点は別更新。
    bool IsChoosingUpgrade() const
    {
        return m_progression.IsChoosing();
    }

    bool IsGameplayPaused() const
    {
#ifdef _DEBUG
        if (m_debugPaused)
        {
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
    bool IsDebugPaused() const { return m_debugPaused; }
    bool DebugGiveUpgrade(UpgradeId id);
#endif
    void Heal(int amount);

    float GetMissileTime()
    {
        return m_MissileTime;
    }

    float GetCurrentMissileTime()
    {
        return m_currentMissileTime;
    }

    void CreateOrbs(int count);

    //debug用
    int getCurRocket()
    {
        return curRocket;
    }

    void SetCurRocket(int num)
    {

        //debug用にミサイルの最大数を設定する
        if (num < 1)
        {
            num = 1;
        }
        if (num > maxRocket)
        {
            num = maxRocket;
        }
        curRocket = num;
    }

    void SetHP(int num)
    {
        m_hp = num;
    }

    Collision::Sphere GetCollisionSphere() override
    {
        return {m_Position, 10.0f}; //半径10
    }

    void TakeDamage(int damage);
};
