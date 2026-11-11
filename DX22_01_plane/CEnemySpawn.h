/// @file CEnemySpawn.h
/// @brief 経過時間に応じた敵の出現と、同時出現数の管理。

#pragma once
#include "Object.h"
#include <memory>
class BoarModel;

class CEnemySpawn : public Object {
private:
    /// 難易度が1段階上がるまでの生存時間（秒）。
    float m_waveSeconds = 45.0f;

    /// 難易度が上がっても維持する出現間隔の下限（秒）。
    float m_minSpawnInterval = 0.75f;

    /// 難易度1段階につき1/この値だけ出現間隔を短縮する。
    float m_intervalReductionDivisor = 6.0f;

    /// 0～99の出現抽選でBoarを選ぶ上限。
    int m_boarThreshold = 20;

    /// Boar以外の抽選でキノコを選ぶ累積上限。
    int m_mushroomThreshold = 35;

    /// キノコまでの種類以外でドローンを選ぶ累積上限。
    int m_droneThreshold = 55;

    /// スライムのうち高速型を選ぶ累積上限。
    int m_runnerThreshold = 65;

    /// 2波目以降で大型スライムを選ぶ累積上限。
    int m_bruteThreshold = 80;

    /// ドローンの出現時の高さ。
    float m_droneSpawnHeight = 40.0f;

    /// スライムの出現時の高さ。接地処理で地面へ合わせる。
    float m_slimeSpawnHeight = -3.0f;

    /// 前回の生成からの経過秒数。
    float m_spawnElapsed = 0.0f;
    /// 出現時にモデル読込で止まらないよう、共有素材を保持する。
    std::unique_ptr<BoarModel> m_boarResources;

public:
#ifdef _DEBUG
    /// 敵を増やさずに武器の動作を観察するための切り替え。
    bool m_debugSpawnEnabled = true;
#endif

    /// 敵を出現させる間隔（秒）。
    float m_spawnInterval = 2.0f;

    /// 出現する円の最小半径
    float m_radiusMin = 500.0f;
    /// 出現する円の最大半径
    float m_radiusMax = 1000.0f;

    /// 出現させる敵の数
    int m_enemyCount = 5;

    /// 敵の最大表示数
    int m_maxEnemyCount = 30;

    CEnemySpawn();
    ~CEnemySpawn();
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
};
