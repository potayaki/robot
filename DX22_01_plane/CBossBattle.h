/// @file CBossBattle.h
/// @brief 通常戦とボス戦の切り替え、バリア、ボスHP表示。

#pragma once
#include "Object.h"
#include "BossBattleRules.h"
#include "BossCatalog.h"
#include "ForestLayout.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include <optional>

class CBossEnemy;
class CPlayer;

// ステージごとに1個生成し、通常戦と一騎打ちの切り替えを管理する。
class CBossBattle : public Object {
    /// 通常戦とボス戦の進行、および出現条件の達成状態。
    BossBattleProgress m_progress;
    /// 次に出現するボスと、このプレイで撃破した種類の記録。
    BossRoster m_roster;
    /// ステージで固定する種類。未指定なら従来の順番で全種類を巡回する。
    std::optional<BossKind> m_stageBossKind;
    /// 前回更新時の部屋。エリア移動時の撃破数リセットに使う。
    ForestLayout::Area m_currentArea = ForestLayout::Area::Entrance;
    /// 戦闘開始時に固定した部屋。ボス戦中の門の通り抜けを防ぐ。
    ForestLayout::Area m_battleArea = ForestLayout::Area::Entrance;
    /// 現在戦闘中の種類。敵が削除された後もUIと撃破処理で参照する。
    BossKind m_activeKind = BossKind::Slime;
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
    /// 次の1戦の種類をデバッグ画面で指定したか。
    bool m_debugBossSelected = false;
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
    void OnBossDefeated(BossKind kind);

    /// 通常の出現種類を指定する。デバッグの選択は次の1戦だけ優先する。
    void SetStageBoss(BossKind kind) {
        m_stageBossKind = kind;
        m_roster.SelectNext(kind);
    }

    int GetDefeatedBossTypes() const {
        return m_roster.DefeatedCount();
    }

    bool IsBattleActive() const {
        return m_progress.IsActive();
    }

    CBossEnemy* GetBoss() const;
    DirectX::SimpleMath::Vector3 ConstrainPosition(const DirectX::SimpleMath::Vector3& position,
                                                   float bodyRadius) const;
};
