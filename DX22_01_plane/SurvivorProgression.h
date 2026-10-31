#pragma once
#include <array>
#include <random>
#include <vector>
#include <string>

// 描画やDirectXから独立した成長ルール。抽選・上限・複数レベル分の選択をここで管理する。
enum class UpgradeId
{
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
struct UpgradeDefinition
{
    UpgradeId id;
    const char8_t* name;
    const char8_t* description;
    int maxRank;
};

const UpgradeDefinition& GetUpgradeDefinition(UpgradeId id);
// 所持一覧と報酬候補の両方に、指定レベルの具体的な性能を表示する。
std::string GetUpgradeEffectText(UpgradeId id, int rank);

class SurvivorProgression
{
    // m_levelはプレイヤーのレベル、m_ranksは各スキルの強化レベル。
    // 撃破で前者が上がり、報酬を選択・取得した時点で後者が上がる。
    std::array<int, static_cast<size_t>(UpgradeId::Count)> m_ranks{};
    std::mt19937 m_random;
    int m_level = 1;
    int m_experience = 0; // 100で経験値1。経験値ボーナスの小数部分も失わない。
    int m_pendingChoices = 0; // まだ選んでいないレベルアップ報酬の回数。
    std::vector<UpgradeId> m_choices; // 現在表示している最大3つの候補。

    // 上限未達の強化を集める処理と、表示用に3つ抽選する処理を分けている。
    std::vector<UpgradeId> EligibleUpgrades() const;
    void RefreshChoices();
    void Grant(UpgradeId id);

public:
    // 通常プレイではランダムなseedを使い、テストでは固定seedで抽選を再現できる。
    explicit SurvivorProgression(unsigned seed = std::random_device{}()) : m_random(seed)
    {
    }

    int Rank(UpgradeId id) const;

    int Level() const
    {
        return m_level;
    }

    int Experience() const
    {
        return m_experience;
    }

    int RequiredExperience() const
    {
        return (5 + (m_level - 1) * 3) * 100;
    }

    int PendingChoices() const
    {
        return m_pendingChoices;
    }

    bool IsChoosing() const
    {
        return m_pendingChoices > 0;
    }

    const std::vector<UpgradeId>& Choices() const
    {
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
