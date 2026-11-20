/// @file CEnemySpawn.h
/// @brief 経過時間に応じた敵の出現と、同時出現数の管理。

#pragma once
#include "Object.h"
#include "EnemySpawnRules.h"
#include <memory>
class BoarModel;
class GhostModel;
class TestModel;
class MushroomModel;

class CEnemySpawn : public Object {
private:
    /// 難易度が1段階上がるまでの生存時間（秒）。
    float m_waveSeconds = EnemySpawning::RampStageSeconds;

    /// 難易度が上がっても維持する出現間隔の下限（秒）。
    float m_minSpawnInterval = EnemySpawning::MinimumInterval;

    /// 難易度1段階につき1/この値だけ出現間隔を短縮する。
    float m_intervalReductionDivisor = 1.0f / EnemySpawning::IntervalReduction;

    /// 0～99の出現抽選でBoarを選ぶ上限。
    int m_boarThreshold = 20;

    /// Boar以外の抽選でキノコを選ぶ累積上限。
    int m_mushroomThreshold = 35;

    /// Ghostを選ぶ累積上限。35～44の10%を支援型にする。
    int m_ghostThreshold = 45;

    /// キノコまでの種類以外でドローンを選ぶ累積上限。
    int m_droneThreshold = 55;

    /// スライムのうち高速型を選ぶ累積上限。
    int m_runnerThreshold = 65;

    /// 2波目以降で大型スライムを選ぶ累積上限。
    int m_bruteThreshold = 80;

    /// ドローンの出現時の高さ。
    float m_droneSpawnHeight = 40.0f;

    /// 地面オブジェクトがない場合のスライムの足元高さ。
    float m_slimeSpawnHeight = -3.0f;

    /// 前回の生成からの経過秒数。
    float m_spawnElapsed = 0.0f;
    /// 出現時にモデル読込で止まらないよう、共有素材を保持する。
    std::unique_ptr<BoarModel> m_boarResources;
    /// 出現時のFBX読込を避けるため、Ghostの描画資源を保持する。
    std::shared_ptr<GhostModel> m_ghostResources;
    /// 群れを追加するたびにスライムのFBX・画像を読み直さないための保持用モデル。
    std::unique_ptr<TestModel> m_slimeResources;
    /// 野営地へ移動しても飛行型の共有描画資源を読み直さないための保持用モデル。
    std::unique_ptr<TestModel> m_flyResources;
    /// キノコの共有描画資源をステージ終了まで保持する。
    std::unique_ptr<MushroomModel> m_mushroomResources;

public:
    /// このステージの通常敵をスライムの各種に限定する。
    bool m_slimesOnly = false;

#ifdef _DEBUG
    /// 敵を増やさずに武器の動作を観察するための切り替え。
    bool m_debugSpawnEnabled = true;
    /// 新しく出す敵の色・強さを固定する。-1は通常の抽選、0～2は通常・亜種・希少種。
    int m_debugRarity = -1;
#endif

    /// 開始時の出現間隔（秒）。生存時間に応じて徐々に短縮する。
    float m_spawnInterval = EnemySpawning::InitialInterval;

    /// プレイヤーを中心とする出現範囲の最小水平半径（ワールド座標）。
    float m_radiusMin = EnemySpawning::SpawnRadiusMin;
    /// プレイヤーを中心とする出現範囲の最大水平半径（ワールド座標）。
    float m_radiusMax = EnemySpawning::SpawnRadiusMax;

    /// 開始時に1回で出現させる数。時間とともに増加し、同時出現枠を超えない。
    int m_enemyCount = EnemySpawning::InitialPackSize;

    /// 開始時の同時出現上限。時間とともに増やす。
    int m_maxEnemyCount = EnemySpawning::InitialMaxAlive;

    /// 森の入口で亜種を選ぶ確率（%）。奥の部屋ごとに5ポイント増やす。
    int m_variantPercent = 20;
    /// 森の入口で希少種を選ぶ確率（%）。奥の部屋ごとに2ポイント増やす。
    int m_rarePercent = 3;

    CEnemySpawn();
    ~CEnemySpawn();
    /// 一時停止を除く生存時間（秒）から、現在の出現数・上限・間隔を求める。
    EnemySpawning::Pressure GetPressure(double elapsedSeconds) const;
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
};
