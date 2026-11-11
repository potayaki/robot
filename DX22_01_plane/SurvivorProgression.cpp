/// @file SurvivorProgression.cpp
/// @brief レベル・経験値・スキル抽選と、強化内容の表示用データ。

#include "SurvivorProgression.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "BulletHoming.h"

namespace {
    // レベルアップとプレゼントは同じ候補表を使い、最大レベルの項目は抽選から外す。
    // 1項目の並びは「ID / 表示名 / 説明 / 最大レベル」。
    // 配列の添字で参照するため、UpgradeIdの順序と一致させる。
    const std::array<UpgradeDefinition, static_cast<size_t>(UpgradeId::Count)> Definitions = {
        {{UpgradeId::BulletPower, u8"強装弾", u8"通常弾のダメージ +1。左クリックの射撃を強化。", 5},
         {UpgradeId::RapidFire, u8"高速装填", u8"通常弾の発射間隔を15%短縮。", 4},
         {UpgradeId::MultiShot, u8"マルチショット", u8"通常弾の同時発射数 +1。照準方向に加えて扇状に発射。",
          2},
         {UpgradeId::MissilePower, u8"強化ミサイル", u8"ミサイルのダメージ +2。右クリックの攻撃を強化。", 4},
         {UpgradeId::MissileVolley, u8"ミサイルポッド", u8"ミサイルの同時発射数 +1。ロックした敵へ分配。", 4},
         {UpgradeId::MissileReload, u8"ミサイル冷却", u8"ミサイルの発射間隔を15%短縮。", 4},
         {UpgradeId::Orbit, u8"守護オーブ", u8"自動武器。周囲を回るオーブを展開。強化で個数・威力が増加。",
          5},
         {UpgradeId::Turret, u8"自動砲台",
          u8"自動武器。後方をついてくる目型の相棒を1体召喚。強化ごとに攻撃力・連射速度・射程が上昇。", 5},
         {UpgradeId::Shockwave, u8"衝撃波",
          u8"自動武器。周囲の敵へ周期的に範囲ダメージ。強化で範囲・威力が増加。", 5},
         {UpgradeId::Lightning, u8"連鎖電撃", u8"自動武器。近くの敵へ電撃を連鎖。飛行する敵にも命中。", 5},
         {UpgradeId::MoveSpeed, u8"機動力", u8"移動速度 +10%。", 5},
         {UpgradeId::Vitality, u8"強化装甲", u8"最大HP +20、現在HPも20回復。", 5},
         {UpgradeId::Armor, u8"ダメージ軽減", u8"受けるダメージ -1。最低1ダメージは受ける。", 3},
         {UpgradeId::Regeneration, u8"自動修復", u8"3秒ごとにHPを2回復。強化するたび回復量 +2。", 5},
         {UpgradeId::Experience, u8"学習回路", u8"撃破時に得る経験値 +20%。", 5},
         {UpgradeId::BulletHoming, u8"誘導弾",
          u8"弾用カーソルの枠内で、照準の中心に最も近い敵へ通常弾が少し曲がる。レベルが上がると枠が広がり、旋"
          u8"回も強くなる。外れた先の枠内の敵にも再誘導。",
          BulletHoming::MaxRank}}};

    // 通常のスキルとは別枠で扱う、全強化完了後の回復報酬。
    const UpgradeDefinition Recovery = {UpgradeId::Heal, u8"応急修理",
                                        u8"HPを25回復。すべての強化が最大になると出現。", 0};
} // namespace

const UpgradeDefinition& GetUpgradeDefinition(UpgradeId id) {
    // Healなど通常の強化表に含まれないIDは、回復報酬の定義を返す。
    const auto index = static_cast<size_t>(id);
    return index < Definitions.size() ? Definitions[index] : Recovery;
}

TurretStats GetTurretStats(int rank) {
    rank = std::clamp(rank, 1, GetUpgradeDefinition(UpgradeId::Turret).maxRank);
    return {rank, UpgradeBalance::TurretBaseRange + UpgradeBalance::TurretRangePerRank * rank,
            UpgradeBalance::TurretBaseInterval - UpgradeBalance::TurretIntervalPerRank * rank};
}

std::string GetUpgradeEffectText(UpgradeId id, int rank) {
    rank = std::clamp(rank, 0, GetUpgradeDefinition(id).maxRank);
    char text[512]{};

    // u8文字列を、ImGuiが受け取るUTF-8のchar文字列として整形する。
    // ここでは表示だけを行い、実際の攻撃力やHPは書き換えない。
    const auto format = [&](const char8_t* pattern, auto... values) {
        std::snprintf(text, sizeof(text), reinterpret_cast<const char*>(pattern), values...);
    };

    // 武器の計算式と同じ値を表示し、「Lv」だけでは分からない実際の効果を補足する。
    // 発射間隔はゲーム側と同じ秒単位の式で表示する。
    switch (id) {
        case UpgradeId::BulletPower:
            format(u8"通常弾ダメージ：%d（基本1 + %d）", 1 + rank, rank);
            break;

        case UpgradeId::RapidFire:
            format(u8"通常弾の平均発射間隔：%.3f秒",
                   UpgradeBalance::BulletInterval * std::pow(UpgradeBalance::ReloadMultiplier, rank));
            break;

        case UpgradeId::MultiShot:
            format(u8"1回の射撃で%d発（中央の弾は照準方向）", 1 + rank);
            break;

        case UpgradeId::MissilePower:
            format(u8"ミサイルダメージ：%d（基本3 + %d）",
                   UpgradeBalance::BaseMissileDamage + UpgradeBalance::MissileDamagePerRank * rank,
                   UpgradeBalance::MissileDamagePerRank * rank);
            break;

        case UpgradeId::MissileVolley:
            format(u8"ミサイル同時発射数：%d発", 1 + rank);
            break;

        case UpgradeId::MissileReload:
            format(u8"ミサイルの平均発射間隔：%.2f秒",
                   UpgradeBalance::MissileInterval * std::pow(UpgradeBalance::ReloadMultiplier, rank));
            break;

        case UpgradeId::Orbit:
            format(u8"オーブ%d個 / ダメージ%d / 周回半径50", 1 + rank, 1 + rank);
            break;

        case UpgradeId::Turret: {
            const auto stats = GetTurretStats(rank);
            format(u8"追従する相棒1体 / ダメージ%d / 射程%.0f / 攻撃間隔%.2f秒", stats.damage, stats.range,
                   stats.attackInterval);
            break;
        }

        case UpgradeId::Shockwave:
            format(u8"半径%d / ダメージ%d / 発動間隔%.1f秒",
                   UpgradeBalance::ShockwaveBaseRadius + UpgradeBalance::ShockwaveRadiusPerRank * rank,
                   UpgradeBalance::BaseAutoWeaponDamage + rank,
                   UpgradeBalance::ShockwaveBaseInterval - UpgradeBalance::ShockwaveIntervalPerRank * rank);
            break;

        case UpgradeId::Lightning:
            format(u8"最大%d体へ連鎖 / ダメージ%d / 射程350 / 発動間隔%.2f秒",
                   UpgradeBalance::LightningBaseTargets + rank, UpgradeBalance::BaseAutoWeaponDamage + rank,
                   UpgradeBalance::LightningBaseInterval - rank / UpgradeBalance::LightningIntervalDivisor);
            break;

        case UpgradeId::MoveSpeed:
            format(u8"移動速度：基本の%d%%（+%d%%）", 100 + UpgradeBalance::MoveBonusPercent * rank,
                   UpgradeBalance::MoveBonusPercent * rank);
            break;

        case UpgradeId::Vitality:
            format(u8"最大HP：%d（+%d）/ 取得時にHP20回復",
                   UpgradeBalance::BaseMaxHp + UpgradeBalance::VitalityPerRank * rank,
                   UpgradeBalance::VitalityPerRank * rank);
            break;

        case UpgradeId::Armor:
            format(u8"1回の被ダメージ -%d（最低1）", rank);
            break;

        case UpgradeId::Regeneration:
            format(u8"%.0f秒ごとにHP%d回復（最大HPまで）", UpgradeBalance::RegenerationInterval,
                   UpgradeBalance::RegenerationHpPerRank * rank);
            break;

        case UpgradeId::Experience:
            format(u8"撃破経験値：基本の%d%%（+%d%%）",
                   UpgradeBalance::ExperienceUnit + UpgradeBalance::ExperienceBonusPercent * rank,
                   UpgradeBalance::ExperienceBonusPercent * rank);
            break;

        case UpgradeId::BulletHoming:
            format(u8"対象：弾用カーソルの中心に最も近い敵 / 枠サイズ：%.0f / 最大旋回：%.0f度/秒 / "
                   u8"弾からの探索半径：%.0f",
                   BulletHoming::CrosshairSize(rank), BulletHoming::DegreesPerSecond(rank),
                   BulletHoming::SearchRadius);
            break;

        case UpgradeId::Heal:
            format(u8"HP25回復（最大HPまで）");
            break;

        default:
            break;
    }

    return text;
}

int SurvivorProgression::Rank(UpgradeId id) const {
    const auto index = static_cast<size_t>(id);
    return index < m_ranks.size() ? m_ranks[index] : 0;
}

std::vector<UpgradeId> SurvivorProgression::EligibleUpgrades() const {
    std::vector<UpgradeId> candidates;

    // 同じスキルは表に1つだけなので、この時点で候補の重複は発生しない。
    for (const auto& definition : Definitions) {
        if (Rank(definition.id) < definition.maxRank) {
            candidates.push_back(definition.id);
        }
    }

    // 全項目が最大でも、報酬画面を空にせず回復を選べるようにする。
    if (candidates.empty()) {
        candidates.push_back(UpgradeId::Heal);
    }

    return candidates;
}

void SurvivorProgression::RefreshChoices() {
    m_choices.clear();
    if (!IsChoosing()) {
        return;
    }

    // 候補全体をシャッフルして先頭3つを使う。候補が1～2個なら、その数だけ表示。
    m_choices = EligibleUpgrades();
    std::shuffle(m_choices.begin(), m_choices.end(), m_random);
    if (m_choices.size() > UpgradeBalance::ChoiceCount) {
        m_choices.resize(UpgradeBalance::ChoiceCount);
    }
}

void SurvivorProgression::Grant(UpgradeId id) {
    // 回復はスキルではないのでランクを増やさない。HP回復自体はCPlayer側で行う。
    if (id != UpgradeId::Heal) {
        ++m_ranks[static_cast<size_t>(id)];
    }
}

void SurvivorProgression::AddExperience(int amount) {
    if (amount <= 0) {
        return;
    }

    // 例：経験値1、学習回路Lv1なら120を加算する。内部の100が画面上の経験値1。
    m_experience += amount * (UpgradeBalance::ExperienceUnit +
                              Rank(UpgradeId::Experience) * UpgradeBalance::ExperienceBonusPercent);

    // 一度に大量の経験値を得た場合は、条件を満たす限りレベルを上げる。
    // 必要経験値を引いた残りは次のレベルへ持ち越し、報酬の選択回数を記録する。
    while (m_experience >= RequiredExperience()) {
        m_experience -= RequiredExperience();
        ++m_level;
        ++m_pendingChoices;
    }

    // 同じフレームに複数の敵を倒しても、提示済みの候補は勝手に変えない。
    if (IsChoosing() && m_choices.empty()) {
        RefreshChoices();
    }
}

bool SurvivorProgression::Select(size_t index, UpgradeId& granted) {
    // 候補の範囲外や選択待ちでない場合は、何も変更しない。
    if (!IsChoosing() || index >= m_choices.size()) {
        return false;
    }

    // 1回の選択につき1つ強化し、続けて選ぶ報酬があれば候補を抽選し直す。
    granted = m_choices[index];
    Grant(granted);
    --m_pendingChoices;
    RefreshChoices();
    return true;
}

#ifdef _DEBUG
bool SurvivorProgression::DebugGrantUpgrade(UpgradeId id) {
    // Healや不正なIDはランク配列に入れない。最大レベルを超える取得も拒否する。
    const auto index = static_cast<size_t>(id);
    if (index >= m_ranks.size() || Rank(id) >= GetUpgradeDefinition(id).maxRank) {
        return false;
    }

    Grant(id);
    // 選択待ちがある場合、最大になったスキルを報酬候補に残さない。
    if (IsChoosing()) {
        RefreshChoices();
    }
    return true;
}
#endif

UpgradeId SurvivorProgression::GrantPresent() {
    // レベルアップと同じ候補から、均等な確率で1つ選んで即時取得する。
    const auto candidates = EligibleUpgrades();
    std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
    const auto granted = candidates[pick(m_random)];
    Grant(granted);

    // プレゼントで上限に達した候補を、その後のレベルアップ画面に残さない。
    if (IsChoosing()) {
        RefreshChoices();
    }
    return granted;
}
