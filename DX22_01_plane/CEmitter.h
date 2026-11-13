/// @file CEmitter.h
/// @brief 粒子プールから発射位置の閃光と火花をまとめて生成する。

#pragma once
#include "CParticle.h"
#include <random>

class CEmitter {
private:
    /// 1回の射撃で前方へ散らす火花の数。
    int m_sparkCount = 10;
    /// 火花の広がり。前方速度に対する横方向の速度の最大比率。
    float m_spread = 0.65f;
    /// 火花の初速の範囲（ワールド単位/秒）。
    float m_minSpeed = 45.0f;
    /// 火花の初速の上限（ワールド単位/秒）。
    float m_maxSpeed = 95.0f;
    /// 火花が消えるまでの最短時間（秒）。
    float m_minLife = 0.10f;
    /// 火花が消えるまでの最長時間（秒）。
    float m_maxLife = 0.22f;
    /// 火花の板の大きさ（ワールド単位）。
    float m_sparkSize = 2.5f;
    /// 発射位置に出す閃光の大きさ（ワールド単位）。
    float m_flashSize = 9.0f;
    /// 発射位置の閃光が消えるまでの時間（秒）。
    float m_flashLife = 0.07f;
    /// 他のゲーム処理の乱数列へ影響させない発生器。
    std::mt19937 m_random{std::random_device{}()};

public:
    /// ワールド座標の発射位置から、射撃方向へ1回分の粒子を放出する。
    void Emit(ParticleManager& particles, const DirectX::SimpleMath::Vector3& position,
              const DirectX::SimpleMath::Vector3& direction);
};
