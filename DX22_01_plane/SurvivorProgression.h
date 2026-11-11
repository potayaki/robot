/// @file SurvivorProgression.h
/// @brief レベル・経験値・スキル抽選と、強化内容の表示用データ。

#pragma once
#include <array>
#include <random>
#include <vector>
#include <string>

// 実際の攻撃・成長と、装備画面の説明が同じ調整値を参照する。
// 固定のゲームルールなので個々のプレイヤーには複製せず、名前付き定数として共有する。
namespace UpgradeBalance {
    /// 画面上の経験値1に対応する内部値。百分率の端数を整数で保持する。
    inline constexpr int ExperienceUnit = 100;
    /// レベル1から2に上がるために必要な経験値。
    inline constexpr int InitialRequiredExperience = 5;
    /// レベルごとに増える必要経験値。
    inline constexpr int RequiredExperiencePerLevel = 3;
    /// 一度に提示するレベルアップ報酬の最大数。
    inline constexpr size_t ChoiceCount = 3;
    /// 経験値スキル1段階あたりの加算率（百分率）。
    inline constexpr int ExperienceBonusPercent = 20;
    /// 強化前の最大HP。
    inline constexpr int BaseMaxHp = 100;
    /// 装甲1段階で増える最大HPと、取得時の回復量。
    inline constexpr int VitalityPerRank = 20;
    /// 全スキル取得後の回復報酬で回復するHP。
    inline constexpr int RecoveryHp = 25;
    /// 機動力1段階あたりの速度加算率（百分率）。
    inline constexpr int MoveBonusPercent = 10;
    /// 移動・歩行アニメーションに使用する速度の加算倍率。
    inline constexpr float MoveBonusPerRank = 0.1f;
    /// 強化前の通常弾ダメージ。
    inline constexpr int BaseBulletDamage = 1;
    /// 強化前のミサイルダメージ。
    inline constexpr int BaseMissileDamage = 3;
    /// ミサイル強化1段階あたりの追加ダメージ。
    inline constexpr int MissileDamagePerRank = 2;
    /// 通常弾の初期発射間隔（秒）。
    inline constexpr float BulletInterval = 0.1f;
    /// ミサイルの初期発射間隔（秒）。
    inline constexpr float MissileInterval = 2.0f;
    /// 装填・冷却1段階で発射間隔に掛ける倍率。
    inline constexpr float ReloadMultiplier = 0.85f;
    /// オーブ・衝撃波・連鎖電撃のランク加算前ダメージ。
    inline constexpr int BaseAutoWeaponDamage = 1;
    /// オーブの周回半径。説明文と初期設定で共有する。
    inline constexpr float OrbitRadius = 50.0f;
    /// 砲台のランク加算前の射程。
    inline constexpr float TurretBaseRange = 400.0f;
    /// 砲台のランクごとに増える射程。
    inline constexpr float TurretRangePerRank = 40.0f;
    /// 砲台のランク短縮前の攻撃間隔（秒）。
    inline constexpr float TurretBaseInterval = 0.5f;
    /// 砲台のランクごとに短縮する攻撃間隔（秒）。
    inline constexpr float TurretIntervalPerRank = 0.05f;
    /// 衝撃波のランク加算前の半径。
    inline constexpr int ShockwaveBaseRadius = 80;
    /// 衝撃波のランクごとに増える半径。
    inline constexpr int ShockwaveRadiusPerRank = 12;
    /// 衝撃波のランク短縮前の間隔（秒）。
    inline constexpr float ShockwaveBaseInterval = 3.0f;
    /// 衝撃波のランクごとに短縮する間隔（秒）。
    inline constexpr float ShockwaveIntervalPerRank = 0.2f;
    /// 電撃のランク加算前の連鎖対象数。
    inline constexpr int LightningBaseTargets = 2;
    /// 電撃が最初の敵を探す距離。
    inline constexpr float LightningRange = 350.0f;
    /// 電撃が次の敵へ連鎖できる距離。
    inline constexpr float LightningChainRange = 140.0f;
    /// 電撃のランク短縮前の間隔（秒）。
    inline constexpr float LightningBaseInterval = 2.5f;
    /// 電撃の短縮秒数を計算する除数。従来のrank / 6を保つ。
    inline constexpr float LightningIntervalDivisor = 6.0f;
    /// 自動修復が発動する間隔（秒）。
    inline constexpr float RegenerationInterval = 3.0f;
    /// 自動修復1段階あたりの回復量。
    inline constexpr int RegenerationHpPerRank = 2;
} // namespace UpgradeBalance

// 描画やDirectXから独立した成長ルール。抽選・上限・複数レベル分の選択をここで管理する。
enum class UpgradeId {
    BulletPower, // 通常弾の威力
    RapidFire, // 通常弾の連射速度
    MultiShot, // 通常弾の同時発射数
    MissilePower, // ミサイルの威力
    MissileVolley, // ミサイルの同時発射数
    MissileReload, // ミサイルの発射間隔
    Orbit, // 周回オーブ
    Turret, // 自動砲台
    Shockwave, // 周囲への範囲攻撃
    Lightning, // 敵から敵へ連鎖する電撃
    MoveSpeed, // プレイヤーの移動速度
    Vitality, // 最大HP
    Armor, // 被ダメージの軽減
    Regeneration, // 自動回復
    Experience, // 撃破経験値の増加
    BulletHoming, // 通常弾の誘導
    Count, // 通常の強化の種類数。ランク配列のサイズに使用する。
    Heal // 全強化が最大のときだけ使う回復報酬。ランクは持たない。
};

// 候補名・説明・上限をまとめた共通データ。
// 各要素の並びはSurvivorProgression.cppのDefinitionsと対応させる。
struct UpgradeDefinition {
    /// 強化の種類を識別するID。
    UpgradeId id;
    /// 報酬候補や装備一覧へ表示する強化の名前。
    const char8_t* name;
    /// 強化内容を説明する表示文。
    const char8_t* description;
    /// この強化を取得できる最大レベル。
    int maxRank;
};

const UpgradeDefinition& GetUpgradeDefinition(UpgradeId id);

// 相棒砲台の実性能と説明文が同じ値を参照する。
struct TurretStats {
    /// 攻撃が命中したときに与えるダメージ量。
    int damage;
    /// 相棒砲台が敵を探す攻撃範囲の半径。
    float range;
    /// 相棒砲台が攻撃する間隔（秒）。
    float attackInterval;
};

TurretStats GetTurretStats(int rank);
// 所持一覧と報酬候補の両方に、指定レベルの具体的な性能を表示する。
std::string GetUpgradeEffectText(UpgradeId id, int rank);

class SurvivorProgression {
    // m_levelはプレイヤーのレベル、m_ranksは各スキルの強化レベル。
    // 撃破で前者が上がり、報酬を選択・取得した時点で後者が上がる。
    /// 強化の種類ごとに現在の取得レベルを保持する配列。
    std::array<int, static_cast<size_t>(UpgradeId::Count)> m_ranks{};
    /// 抽選を行う乱数生成器。
    std::mt19937 m_random;
    /// プレイヤーの現在のレベル。
    int m_level = 1;
    /// 100で経験値1。経験値ボーナスの小数部分も失わない。
    int m_experience = 0;
    /// まだ選んでいないレベルアップ報酬の回数。
    int m_pendingChoices = 0;
    /// 現在表示している最大3つの候補。
    std::vector<UpgradeId> m_choices;

    // 上限未達の強化を集める処理と、表示用に3つ抽選する処理を分けている。
    std::vector<UpgradeId> EligibleUpgrades() const;
    void RefreshChoices();
    void Grant(UpgradeId id);

public:
    // 通常プレイではランダムなseedを使い、テストでは固定seedで抽選を再現できる。
    explicit SurvivorProgression(unsigned seed = std::random_device{}()) : m_random(seed) {}

    int Rank(UpgradeId id) const;

    int Level() const {
        return m_level;
    }

    int Experience() const {
        return m_experience;
    }

    int RequiredExperience() const {
        return (UpgradeBalance::InitialRequiredExperience +
                (m_level - 1) * UpgradeBalance::RequiredExperiencePerLevel) *
               UpgradeBalance::ExperienceUnit;
    }

    int PendingChoices() const {
        return m_pendingChoices;
    }

    bool IsChoosing() const {
        return m_pendingChoices > 0;
    }

    const std::vector<UpgradeId>& Choices() const {
        return m_choices;
    }

    // 経験値は通常の単位で渡す（例：1）。内部で100倍して端数を保持する。
    void AddExperience(int amount);

    // indexは表示中の候補番号（0始まり）。成功時はgrantedに取得内容を返す。
    bool Select(size_t index, UpgradeId& granted);

    // プレゼントは選択待ち回数を減らさず、その場でランダムな報酬を1つ与える。
    UpgradeId GrantPresent();

#ifdef _DEBUG
    // 抽選を待たずに指定スキルを1段階取得する。経験値・選択待ち回数は消費しない。
    bool DebugGrantUpgrade(UpgradeId id);
#endif
};
