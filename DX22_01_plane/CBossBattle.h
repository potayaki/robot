#pragma once
#include "Object.h"
#include "BossBattleRules.h"
#include "Texture.h"
#include "VertexBuffer.h"

class CBossEnemy;
class CPlayer;

// ステージごとに1個生成し、通常戦と一騎打ちの切り替えを管理する。
class CBossBattle : public Object
{
    BossBattleProgress m_progress;
    int m_bossHp = 250;
    float m_arenaRadius = 500.0f;
    // 出現時のプレイヤーとボスの水平距離。バリア半径とは別に調整する。
    float m_bossSpawnDistance = 500.0f;
    float m_victoryNotice = 0.0f;
    bool m_ready = false;
    Texture m_white;
    VertexBuffer<VERTEX_3D> m_buffer;
    std::vector<VERTEX_3D> m_vertices;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
#ifdef _DEBUG
    bool m_requestStart = false;
    bool m_requestDefeat = false;
#endif
    void BeginBattle(CPlayer* player);
    float GetBossSpawnDistance() const;
    void RemoveNormalEnemies();

public:
    ~CBossBattle() override { Uninit(); }
    void Init() override;
    void Update() override;
    void Draw(Camera*) override {} // 半透明の壁は、不透明物の描画後にまとめて描く。
    void Uninit() override;
    void DrawBarrier(Camera* cam);
    void DrawUI();
#ifdef _DEBUG
    void DrawDebugUI();
#endif
    void OnBossDefeated();
    bool IsBattleActive() const { return m_progress.IsActive(); }
    CBossEnemy* GetBoss() const;
    DirectX::SimpleMath::Vector3 ConstrainPosition(
        const DirectX::SimpleMath::Vector3& position, float bodyRadius) const;
};
