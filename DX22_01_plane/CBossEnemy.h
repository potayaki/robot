/// @file CBossEnemy.h
/// @brief 5種のボスの攻撃、予告表示、共通の被弾と撃破。
#pragma once
#include "EnemyTarget.h"
#include "TestModel.h"
#include "BoarModel.h"
#include "MushroomModel.h"
#include "GhostModel.h"
#include "BossAttackRules.h"
#include <array>
#include <memory>

class CPlayer;

class CBossEnemy : public EnemyTarget {
public:
    using Phase = BossAttackPhase;

    ~CBossEnemy() override {
        Uninit();
    }

    void Init() override;
    void Configure(int hp, const DirectX::SimpleMath::Vector3& position, BossKind kind = BossKind::Slime);
    void Update() override;
    void Draw(Camera* camera) override;
    void DrawEffects(Camera* camera);
    void Uninit() override;
    void OnHit(int& damage) override;

    int GetHp() const {
        return (std::max)(0, m_hp);
    }

    int GetMaxHp() const {
        return m_maxHp;
    }

    BossKind GetKind() const {
        return m_kind;
    }

    Phase GetPhase() const {
        return m_attack.Phase();
    }

    const char8_t* GetAttackNotice() const;

    Collision::Sphere GetCollisionSphere() override {
        return {m_position, BossRadius(m_kind)};
    }

private:
    struct SpiritShot {
        /// 霊弾の中心のワールド座標。
        DirectX::SimpleMath::Vector3 position;
        /// 霊弾の速度（ワールド単位/秒）。
        DirectX::SimpleMath::Vector3 velocity;
        /// 霊弾が消えるまでの秒数。
        float remaining = 4.0f;
    };

    /// 描画・攻撃に使用するボスの種類。
    BossKind m_kind = BossKind::Slime;
    /// 秒数に基づく攻撃フェーズと連続突進の状態。
    BossAttackCycle m_attack;
    /// 現在の体力。
    int m_hp = 250;
    /// HPバーに使用する最大体力。
    int m_maxHp = 250;
    /// 現在のボスの足元にある地表のワールドY座標。
    float m_groundY = -5.0f;
    /// ホバリング・予告点滅に使う経過秒数。
    float m_elapsed = 0.0f;
    /// 次に接触・胞子ダメージを与えられるまでの秒数。
    float m_damageCooldown = 0.0f;
    /// 今回の突進・急降下が既に命中したか。
    bool m_attackHit = false;
    /// 予告開始時に固定した水平方向。
    DirectX::SimpleMath::Vector3 m_chargeDirection{0, 0, 1};
    /// 予告開始時のボスのワールド位置。
    DirectX::SimpleMath::Vector3 m_attackStart{};
    /// 急降下の終点、または霊弾の照準位置。
    DirectX::SimpleMath::Vector3 m_attackTarget{};
    /// 次の退避で移動する左右方向。攻撃ごとに反転する。
    float m_orbitSign = 1.0f;
    /// 予告開始時に固定する3か所の胞子領域中心。
    std::array<DirectX::SimpleMath::Vector3, 3> m_spores{};
    /// 生存中の霊弾。このボスの寿命に追従する。
    std::vector<SpiritShot> m_shots;
    /// スライムとフライの静的モデル。
    std::unique_ptr<TestModel> m_body;
    /// イノシシのアニメーションモデル。
    std::unique_ptr<BoarModel> m_boar;
    /// キノコの点滅対応モデル。
    std::unique_ptr<MushroomModel> m_mushroom;
    /// ゴーストのモデルと霊弾描画資源。
    std::shared_ptr<GhostModel> m_ghost;
    /// フライの着地点とキノコの胞子領域を表示する白画像。
    Texture m_white;
    /// 予告円用の動的頂点バッファ。
    VertexBuffer<VERTEX_3D> m_effectBuffer;
    /// 予告円を両面から描く描画状態。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_effectRasterizer;
    /// 予告円のGPU資源が使用できるか。
    bool m_effectsReady = false;
    void EnterPhase(CPlayer* player);
    void UpdateMovement(CPlayer* player, float seconds);
    void UpdateShots(CPlayer* player, float seconds);
    void UpdateModel(float seconds);
    DirectX::SimpleMath::Vector3 Constrain(const DirectX::SimpleMath::Vector3& position, float radius) const;
};
