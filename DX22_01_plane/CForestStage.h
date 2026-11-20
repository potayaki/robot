/// @brief Stageの自然アセットで構成した森の入口と戦闘広場。
#pragma once
#include "Object.h"
#include "ForestModel.h"
#include "ForestLayout.h"
#include "ForestProgress.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"
#include <memory>

class CForestStage : public Object {
    struct Prop {
        /// 共有モデル配列の番号。
        size_t model;
        /// 地面に接する足元のワールド座標。
        DirectX::SimpleMath::Vector3 feet;
        /// Y軸周りの回転（ラジアン）。
        float yaw;
        /// ワールド単位の全高。
        float height;
    };

    /// 種類ごとに一度だけ読み込む自然モデル。
    std::vector<std::unique_ptr<ForestModel>> m_models;
    /// 固定シードで配置する樹木・岩・草花。
    std::vector<Prop> m_props;
    /// 草地、土の道、石畳を頂点色で表現する平面メッシュ。
    VertexBuffer<VERTEX_3D> m_groundVertices;
    /// 地表面の三角形インデックス。
    IndexBuffer m_groundIndices;
    /// 地表面に使用する照明付き材質。
    Material m_groundMaterial;
    /// 地表面のインデックス数。
    unsigned m_groundIndexCount = 0;
    /// このプレイの開門状態。シーンの再生成で閉じた状態に戻る。
    ForestGateProgress m_gate;
    /// 上へ引き上げて開く門扉の共有モデル。
    std::unique_ptr<ForestModel> m_gateModel;
    void BuildGround();

public:
    ~CForestStage() override {
        Uninit();
    }

    void Init() override;

    void Update() override;

    void OnBossDefeated(BossKind kind, ForestLayout::Area area) {
        m_gate.OnBossDefeated(kind, area);
    }

    bool IsGateUnlocked(size_t index = 0) const {
        return m_gate.IsUnlocked(index);
    }

    bool IsGateOpen(size_t index = 0) const {
        return m_gate.IsOpen(index);
    }

    ForestLayout::Area GetArea(const DirectX::SimpleMath::Vector3& position) const {
        return ForestLayout::GetArea(position, m_gate.OpenGateCount());
    }

    BossKind NextBoss(ForestLayout::Area area) const {
        return m_gate.NextBoss(area);
    }

    bool IsAreaCleared(ForestLayout::Area area) const {
        return m_gate.IsAreaCleared(area);
    }

    bool IsComplete() const {
        return m_gate.IsComplete();
    }

    DirectX::SimpleMath::Vector3 ConstrainMovement(const DirectX::SimpleMath::Vector3& previous,
                                                   const DirectX::SimpleMath::Vector3& requested,
                                                   float radius) const {
        return ForestLayout::ConstrainMovement(previous, requested, radius, m_gate.OpenGateCount());
    }

    void Draw(Camera* camera) override;
    void Uninit() override;
};
