/// @file CFlyingEnemy.h
/// @brief ホバードローンの浮遊、接近、急降下攻撃。

#pragma once

#include "EnemyTarget.h"
#include "TestCube.h"
#include "TestModel.h"
#include <array>
#include <memory>

// 空中で接近し、時々プレイヤーへ急降下するホバードローン。
class CFlyingEnemy : public EnemyTarget {
private:
    /// モデルと合わせたドローンの判定半径。
    float m_collisionRadius = 12.0f;

    /// ホバリングの上下動の位相を進める速度（ラジアン/秒）。
    float m_hoverAngularSpeed = 2.7f;

    /// 簡易モデルの回転翼の角速度（ラジアン/秒）。
    float m_rotorSpeed = 27.0f;

    /// この水平距離以内で急降下を開始する。
    float m_diveStartRange = 130.0f;

    /// 急降下を続ける秒数。
    float m_diveDuration = 55.0f / 60.0f;

    /// 急降下を再開するまでの待ち時間（秒）。
    float m_diveCooldown = 3.0f;

    /// 急降下中の水平速度（ワールド単位/秒）。
    float m_diveSpeed = 168.0f;

    /// 急降下中の上下移動速度の上限。
    float m_verticalSpeed = 150.0f;

    /// 急降下で目指すプレイヤー中心からの高さ。
    float m_diveHeightOffset = 8.0f;

    /// この距離より遠いと接近する。
    float m_approachRange = 75.0f;

    /// この距離より近いと離れる。
    float m_retreatRange = 50.0f;

    /// ホバリング中に近づく速度。
    float m_approachSpeed = 84.0f;

    /// ホバリング中に離れる速度。
    float m_retreatSpeed = 48.0f;

    /// ホバリング中に確保する地面からの高さ。
    float m_groundClearance = 45.0f;

    /// ホバリング中に確保するプレイヤー中心からの高さ。
    float m_playerClearance = 35.0f;

    /// ホバリングの上下動の振幅。
    float m_hoverAmplitude = 3.0f;

    /// 急降下でプレイヤーへ与えるダメージ量。
    int m_contactDamage = 3;

    /// 撃破時に渡す経験値。
    int m_experience = 2;

    /// 簡易ドローンを構成する部品数。
    static constexpr size_t PartCount = 9;
    // FBX が未配置の間だけ、従来の簡易ドローンを表示する。
    /// このオブジェクトの見た目を描画するモデル。
    std::unique_ptr<TestModel> m_model;
    /// モデルを構成する各パーツ。
    std::array<std::unique_ptr<TestCube>, PartCount> m_parts;
    /// 現在の体力。0以下になると死亡処理を行う。
    int m_hp = 4;
    /// 急降下の残り秒数。
    float m_diveTimer = 0.0f;
    /// 次の急降下までの秒数。
    float m_attackCooldown = 5.0f / 3.0f;
    /// 浮遊の上下動を計算するための位相（ラジアン）。
    float m_hoverPhase = 0.0f;
    /// 簡易ドローンのローターの回転角（ラジアン）。
    float m_rotorAngle = 0.0f;
    /// プレゼントのドロップ率
    int m_dropPercent = 2;

public:
    ~CFlyingEnemy() override;
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
    void OnHit(int& damage) override;

    // モデルの大きさを変更したら、当たり判定の半径もここで調整する。
    Collision::Sphere GetCollisionSphere() override {
        return {m_position, m_collisionRadius};
    }
};
