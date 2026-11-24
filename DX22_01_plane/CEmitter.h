/// @file CEmitter.h
/// @brief 粒子プールから銃口の噴炎、着弾の破片、撃破の光を生成する。

#pragma once
#include "CParticle.h"
#include <random>

class CEmitter {
private:
    /// 連射時の噴炎の長さと幅にばらつきを与える、ゲーム判定と独立した乱数。
    std::mt19937 m_random{std::random_device{}()};

public:
    /// ワールド座標の銃口から射撃方向に短い噴炎を1回出す。
    void Emit(ParticleManager& particles, const DirectX::SimpleMath::Vector3& position,
              const DirectX::SimpleMath::Vector3& direction);
    /// 命中した位置に白い衝撃環を出し、入射方向の反対へ角ばった破片を放出する。
    static void EmitImpact(ParticleManager& particles, const DirectX::SimpleMath::Vector3& position,
                           const DirectX::SimpleMath::Vector3& incoming);
    /// 敵の判定球の中心を囲む光を、体格に合わせた半径から外へ広げる。
    static void EmitDefeat(ParticleManager& particles, const Collision::Sphere& sphere);
};
