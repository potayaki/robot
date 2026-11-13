/// @brief 死亡時の記録とゲームオーバー画面。
#pragma once
#include "Scene.h"

struct RunResult {
    /// 生存時間（秒）。停止中と死亡演出の時間は含めない。
    double survivalSeconds = 0.0;
    /// このプレイで倒した敵の数。
    int kills = 0;
    /// 死亡時の到達レベル。
    int level = 1;
    /// このプレイで1回以上倒したボスの種類数。
    int defeatedBossTypes = 0;
    /// 実装済みのボスの全種類数。現在はスライムキングのみ。
    static constexpr int TotalBossTypes = 1;
};

class ResultScene : public Scene {
private:
    /// シーンを破棄しても変化しない死亡時の記録。
    RunResult m_result;

public:
    explicit ResultScene(const RunResult& result) : m_result(result) {}

    void Update() override {}

    void DrawUI() override;
};
