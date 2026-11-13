/// @file CBossBattle.h
/// @brief 通常戦とボス戦の切り替え、バリア、ボスHP表示。

#pragma once
#include "Object.h"
#include "BossBattleRules.h"
#include "Texture.h"
#include "VertexBuffer.h"

class CBossEnemy;
class CPlayer;

// ステージごとに1個生成し、通常戦と一騎打ちの切り替えを管理する。
class CBossBattle : public Object {
    /// 通常戦とボス戦の進行、および出現条件の達成状態。
    BossBattleProgress m_progress;
    /// このプレイでスライムキングを1回以上倒したか。
    bool m_defeatedSlimeKing = false;
    /// 出現するボスに設定する最大HP。
    int m_bossHp = 250;
    /// ボス戦で移動できるバリア内の半径。
    float m_arenaRadius = 500.0f;
    // 出現時のプレイヤーとボスの水平距離。バリア半径とは別に調整する。
    /// ボス出現時のプレイヤーとの水平距離。バリア半径とは独立して調整する。
    float m_bossSpawnDistance = 500.0f;
    /// ボス撃破メッセージを表示する残り秒数。
    float m_victoryNotice = 0.0f;
    /// 必要な描画資源の準備が完了しているか。
    bool m_ready = false;
    /// 頂点色をそのまま描くための白色テクスチャ。
    Texture m_white;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_buffer;
    /// 描画に使用する頂点の配列。
    std::vector<VERTEX_3D> m_vertices;
    /// カリングや塗りつぶし方法を指定する描画設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
#ifdef _DEBUG
    /// デバッグ画面からボス戦の開始が要求されたか。
    bool m_requestStart = false;
    /// デバッグ画面からボス撃破が要求されたか。
    bool m_requestDefeat = false;
#endif
    void BeginBattle(CPlayer* player);
    float GetBossSpawnDistance() const;
    void RemoveNormalEnemies();

public:
    ~CBossBattle() override {
        Uninit();
    }

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;

    /// 現在の状態を画面へ描画する。
    void Draw(Camera*) override {} // 半透明の壁は、不透明物の描画後にまとめて描く。

    /// 使用した資源を解放して終了する。
    void Uninit() override;
    void DrawBarrier(Camera* cam);
    void DrawUI();
#ifdef _DEBUG
    void DrawDebugUI();
#endif
    void OnBossDefeated();

    int GetDefeatedBossTypes() const {
        return m_defeatedSlimeKing ? 1 : 0;
    }

    bool IsBattleActive() const {
        return m_progress.IsActive();
    }

    CBossEnemy* GetBoss() const;
    DirectX::SimpleMath::Vector3 ConstrainPosition(const DirectX::SimpleMath::Vector3& position,
                                                   float bodyRadius) const;
};
