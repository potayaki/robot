#include "CPlayer.h"
#include "imgui.h"
#include <algorithm>
#ifdef _DEBUG
#include "Game.h"
#include "CEnemySpawn.h"
#endif

namespace
{
    // C++20のu8文字列を、ImGuiが使うUTF-8のchar文字列へ渡すための補助関数。
    const char* Text(const char8_t* value)
    {
        return reinterpret_cast<const char*>(value);
    }

    // HUDは表示専用。移動・サイズ保存・入力を無効にし、毎フレーム指定位置へ描画する。
    // 装備一覧だけはNoInputsを外して、詳細を開くボタンを操作できるようにしている。
    constexpr ImGuiWindowFlags HudFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
                                          ImGuiWindowFlags_AlwaysAutoResize;
}

#ifdef _DEBUG
void CPlayer::DrawSurvivorDebugUI()
{
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(12, 245), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(480, (std::max)(240.0f, display.y - 265)), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Skill Debug"))
    {
        ImGui::End();
        return;
    }

    // ウィンドウ全体をスクロールできるため、画面の高さが小さくても全スキルを選べる。
    ImGui::TextUnformatted(Text(u8"スキル名をクリックで取得 / 1段階強化"));
    ImGui::Checkbox(Text(u8"ゲーム一時停止"), &m_debugPaused);
    ImGui::SameLine();
    ImGui::Checkbox(Text(u8"無敵"), &m_debugInvincible);
    if (ImGui::SliderInt("HP", &m_hp, 0, GetMaxHp()))
    {
        m_hp = std::clamp(m_hp, 0, GetMaxHp());
    }

    if (ImGui::CollapsingHeader(Text(u8"経験値・攻撃・敵の確認")))
    {
        ImGui::Text("Lv.%d / EXP %.1f / %.0f / KILLS %d", m_progression.Level(),
                    m_progression.Experience() / 100.0f,
                    m_progression.RequiredExperience() / 100.0f, m_kills);
        ImGui::SliderInt(Text(u8"追加経験値"), &m_debugExperience, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::BeginDisabled(m_hp <= 0 || IsChoosingUpgrade());
        if (ImGui::Button(Text(u8"経験値を追加")))
        {
            // 撃破数やプレゼントの確定ドロップ条件を変えず、経験値だけ追加する。
            m_progression.AddExperience(m_debugExperience);
            if (IsChoosingUpgrade())
            {
                m_selectionDelay = 0.2f;
            }
        }
        ImGui::EndDisabled();

        ImGui::Text(Text(u8"通常弾：残り %.2f 秒 / 間隔 %.3f 秒"),
                    (std::max)(0.0f, m_currentBulletTime), m_BulletTime);
        ImGui::Text(Text(u8"ミサイル：残り %.2f 秒 / 間隔 %.2f 秒"),
                    (std::max)(0.0f, m_currentMissileTime), m_MissileTime);
        ImGui::Text(Text(u8"砲台 %.2f 秒 / 衝撃波 %.2f 秒 / 電撃 %.2f 秒"),
                    (std::max)(0.0f, m_turretTimer), (std::max)(0.0f, m_shockwaveTimer),
                    (std::max)(0.0f, m_lightningTimer));
        if (ImGui::Button(Text(u8"攻撃待ち時間をリセット")))
        {
            // 未取得の自動武器は発動しない。取得済みの武器を次の更新で確認できる。
            m_currentBulletTime = 0;
            m_currentMissileTime = 0;
            m_turretTimer = 0;
            m_shockwaveTimer = 0;
            m_lightningTimer = 0;
        }

        for (auto spawner : Game::GetInstance()->GetObjects<CEnemySpawn>())
        {
            ImGui::PushID(spawner);
            ImGui::Checkbox(Text(u8"敵の自動出現"), &spawner->m_debugSpawnEnabled);
            // 上限を下げても現在の敵は削除せず、次回以降の自動出現を制限する。
            ImGui::SliderInt(Text(u8"敵の出現上限"), &spawner->m_MaxEnemyCount, 1, 60,
                             "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat(Text(u8"基本出現間隔（秒）"), &spawner->m_SpawnInterval, 0.75f, 10.0f,
                               "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TextWrapped("%s", Text(u8"出現間隔は時間経過でも短縮されます。既存の敵は動き続けます。"));
            ImGui::PopID();
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("%s", Text(u8"マウスを重ねると説明と現在の効果を表示"));
    if (m_hp <= 0)
    {
        ImGui::TextUnformatted(Text(u8"HPを上げるとスキルを取得できます。"));
    }
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i)
    {
        const auto id = static_cast<UpgradeId>(i);
        const auto& definition = GetUpgradeDefinition(id);
        const int rank = m_progression.Rank(id);
        const bool maxed = rank >= definition.maxRank;
        ImGui::PushID(i);
        ImGui::BeginDisabled(maxed || m_hp <= 0 || IsChoosingUpgrade());
        if (ImGui::Button(Text(definition.name), ImVec2(210, 30)))
        {
            DebugGiveUpgrade(id);
        }
        ImGui::EndDisabled();
        // 最大レベルでボタンが無効でも、説明は読めるようにする。
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
            ImGui::TextWrapped("%s", Text(definition.description));
            if (rank > 0)
            {
                ImGui::TextWrapped(Text(u8"現在：%s"), GetUpgradeEffectText(id, rank).c_str());
            }
            if (!maxed)
            {
                ImGui::TextWrapped(Text(u8"次のレベル：%s"), GetUpgradeEffectText(id, rank + 1).c_str());
            }
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::Text("Lv.%d / %d%s", m_progression.Rank(id), definition.maxRank,
                    m_progression.Rank(id) >= definition.maxRank ? "  MAX" : "");
        ImGui::PopID();
    }
    ImGui::End();
}
#endif

void CPlayer::DrawSurvivorUI()
{
    // UIの位置とサイズは、現在のウィンドウのピクセル数を基準にする。
    const auto display = ImGui::GetIO().DisplaySize;

    // ----- 1. 詳細画面を開閉する入力 -----
    // Tabで安全に説明を読める。レベルアップ中は報酬選択を優先する。
    if (!IsChoosingUpgrade() && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
    {
        m_showLoadout = !m_showLoadout;
    }

    // ----- 2. 画面上部のレベル・HP・経験値ゲージ -----
    const float width = (std::min)(560.0f, display.x * 0.48f);
    // 経験値は撃破と同時に直接加算する。ゲージで次のレベルまでの残量を示す。
    // 上部中央の既存タイマーと重ならないよう、その下に配置する。
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, 110.0f), 0, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(width, 0));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("SurvivorStatus", nullptr, HudFlags);
    ImGui::Text("Lv.%d   HP %d / %d   KILLS %d", m_progression.Level(), m_hp, GetMaxHp(), m_kills);

    // 経験値は内部で100倍されているので、表示するときだけ元の単位へ戻す。
    char xp[80];
    snprintf(xp,
             sizeof(xp),
             "EXP %.1f / %.0f",
             m_progression.Experience() / 100.0f,
             m_progression.RequiredExperience() / 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.7f, 0.85f, 1));
    ImGui::ProgressBar(static_cast<float>(m_progression.Experience()) / m_progression.RequiredExperience(),
                       ImVec2(-1, 16),
                       xp);
    ImGui::PopStyleColor();
    ImGui::End();

    // ----- 3. 戦闘中に表示する短い装備一覧 -----
    // 戦闘中は短い一覧、詳細は停止画面に分けて視界を塞がないようにする。
    ImGui::SetNextWindowPos(ImVec2(display.x - 16, 160), 0, ImVec2(1, 0));
    ImGui::SetNextWindowSize(ImVec2(310, 0));
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("SurvivorLoadout", nullptr, HudFlags & ~ImGuiWindowFlags_NoInputs);
    ImGui::TextUnformatted(Text(u8"装備 / スキル"));
    ImGui::Separator();
    ImGui::TextUnformatted(Text(u8"通常弾：左クリック"));
    ImGui::TextUnformatted(Text(u8"ミサイル：右クリック"));

    // 一覧は最大6種類まで。所持数は全部数え、残りの数と詳細画面の案内を出す。
    int ownedCount = 0;
    for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i)
    {
        const auto id = static_cast<UpgradeId>(i);
        const int rank = m_progression.Rank(id);
        if (rank > 0)
        {
            const auto& definition = GetUpgradeDefinition(id);
            if (ownedCount < 6)
            {
                ImGui::Text("%s  Lv.%d / %d", Text(definition.name), rank, definition.maxRank);
            }
            ++ownedCount;
        }
    }
    if (ownedCount == 0)
    {
        ImGui::TextDisabled("%s", Text(u8"強化はまだありません"));
    }
    if (ownedCount > 6)
    {
        ImGui::Text(Text(u8"ほか%d種類（詳細ですべて表示）"), ownedCount - 6);
    }

    // 報酬を選んでいる途中に別のモーダル画面を重ねないよう、一時的にボタンを無効にする。
    ImGui::BeginDisabled(IsChoosingUpgrade());
    if (ImGui::Button(Text(u8"Tab：効果の詳細を見る"), ImVec2(-1, 30)))
    {
        m_showLoadout = true;
    }
    ImGui::EndDisabled();
    ImGui::End();

    // ----- 4. 装備・スキルの詳細画面 -----
    // m_showLoadoutはGame::Updateでも参照され、開いている間はゲーム進行を止める。
    // ImGuiのモーダル表示だけではゲーム内の敵・弾・タイマーは停止しない。
    const char* detailsTitle = Text(u8"装備 / スキル詳細###SurvivorLoadoutDetails");
    if (m_showLoadout && !ImGui::IsPopupOpen(detailsTitle))
    {
        ImGui::OpenPopup(detailsTitle);
    }
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2((std::min)(760.0f, display.x - 32), (std::min)(720.0f, display.y - 40)));
    if (ImGui::BeginPopupModal(detailsTitle,
                               nullptr,
                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize))
    {
        // 閉じる場合もBegin/Endを通し、ImGui側のモーダル状態を確実に解除する。
        if (!m_showLoadout)
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::TextColored(
            ImVec4(0.5f, 0.85f, 1, 1), "%s", Text(u8"ゲーム停止中 / Tab または下のボタンで戻る"));
        ImGui::TextUnformatted(
            Text(u8"スクロールで全スキルを確認できます。数値は現在の強化レベルの効果です。"));
        ImGui::Separator();

        // 下側に閉じるボタンの領域を残し、その上だけをスクロールできる領域にする。
        ImGui::BeginChild("LoadoutScroll", ImVec2(0, -45));
        ImGui::TextUnformatted(Text(u8"基本武器（最初から使用可能）"));
        ImGui::Text(Text(u8"通常弾：左クリック / ダメージ%d / %d発 / 平均発射間隔%.3f秒"),
                    1 + m_progression.Rank(UpgradeId::BulletPower),
                    1 + m_progression.Rank(UpgradeId::MultiShot),
                    m_BulletTime);
        ImGui::Text(Text(u8"ミサイル：右クリック / ダメージ%d / %d発 / 平均発射間隔%.2f秒"),
                    3 + 2 * m_progression.Rank(UpgradeId::MissilePower),
                    curRocket,
                    m_MissileTime);
        if (ownedCount == 0)
        {
            ImGui::Spacing();
            ImGui::TextWrapped(
                "%s", Text(u8"敵を倒してレベルアップするか、プレゼントを拾うと武器・スキルを獲得できます。"));
        }

        // 所持している項目だけ、一般的な説明と現在の具体的な性能をセットで表示する。
        for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i)
        {
            const auto id = static_cast<UpgradeId>(i);
            const int rank = m_progression.Rank(id);
            if (rank <= 0)
            {
                continue;
            }
            const auto& definition = GetUpgradeDefinition(id);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(
                ImVec4(0.5f, 0.85f, 1, 1), "%s  Lv.%d / %d", Text(definition.name), rank, definition.maxRank);
            ImGui::TextWrapped("%s", Text(definition.description));
            const auto effect = GetUpgradeEffectText(id, rank);
            ImGui::TextWrapped(Text(u8"現在の効果：%s"), effect.c_str());
        }

        // BeginChild/EndChild、BeginPopupModal/EndPopupは必ず対応させる。
        ImGui::EndChild();
        if (ImGui::Button(Text(u8"閉じてゲームに戻る"), ImVec2(-1, 32)))
        {
            m_showLoadout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ----- 5. 画面下部の「獲得した報酬」通知 -----
    // 説明を読んでいる間に通知が消えないよう、一時停止中は表示時間も減らさない。
    if (m_rewardNoticeTime > 0)
    {
        if (!IsGameplayPaused())
        {
            GameTime::CountDown(m_rewardNoticeTime, GameTime::UnscaledDeltaSeconds());
        }
        ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y - 70), 0, ImVec2(0.5f, 1));
        ImGui::Begin("RewardNotice", nullptr, HudFlags);
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.35f, 1), "%s", m_rewardNotice.c_str());
        ImGui::End();
    }

    // ----- 6. レベルアップ時の最大3択 -----
    // ここから先は報酬の選択待ちがあるときだけ描画する。
    if (!IsChoosingUpgrade())
    {
        return;
    }
    if (m_selectionDelay > 0)
    {
        // 選択待ちはゲーム停止中も実時間で減らし、0秒で入力を解禁する。
        GameTime::CountDown(m_selectionDelay, GameTime::UnscaledDeltaSeconds());
    }
    if (!ImGui::IsPopupOpen("SurvivorLevelUp"))
    {
        ImGui::OpenPopup("SurvivorLevelUp");
    }
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2((std::min)(700.0f, display.x - 32), 0));
    // モーダル表示に加えてGame側でも更新を止め、選択中は攻撃・移動・時間経過を防ぐ。
    if (ImGui::BeginPopupModal("SurvivorLevelUp",
                               nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::TextColored(
            ImVec4(0.5f, 0.85f, 1, 1), "%s  Lv.%d", Text(u8"レベルアップ！"), m_progression.Level());
        ImGui::TextUnformatted(Text(u8"武器・スキルを1つ選択。クリック、または数字キー 1 / 2 / 3"));
        ImGui::Text(Text(u8"選択待ち：%d 回（選択中はゲーム停止）"), m_progression.PendingChoices());
        ImGui::Separator();

        // -1は未選択。クリックと数字キーの結果を、最後の確定処理へまとめて渡す。
        int selected = -1;
        const auto& choices = m_progression.Choices();
        ImGui::BeginDisabled(m_selectionDelay > 0);
        for (size_t i = 0; i < choices.size(); ++i)
        {
            const auto& definition = GetUpgradeDefinition(choices[i]);
            // 各候補を独立したボタンとして識別できるよう、ImGui用のIDを付ける。
            ImGui::PushID(static_cast<int>(i));
            char label[160];
            if (choices[i] == UpgradeId::Heal)
            {
                snprintf(label, sizeof(label), "%d. %s", static_cast<int>(i) + 1, Text(definition.name));
            }
            else
            {
                snprintf(label,
                         sizeof(label),
                         "%d. %s  Lv.%d",
                         static_cast<int>(i) + 1,
                         Text(definition.name),
                         m_progression.Rank(choices[i]) + 1);
            }
            if (ImGui::Button(label, ImVec2(-1, 42)))
            {
                selected = static_cast<int>(i);
            }
            ImGui::TextWrapped("%s", Text(definition.description));

            // 選択前にも、取得後の具体的な性能を確認できるようにする。
            const auto nextEffect = GetUpgradeEffectText(choices[i], m_progression.Rank(choices[i]) + 1);
            ImGui::TextWrapped(Text(u8"取得後：%s"), nextEffect.c_str());
            ImGui::Spacing();

            // UIの入力イベントを使い、フレーム間の短い押下も取りこぼさない。
            // キーリピートは無効にして、長押しで次の報酬まで選ばれることを防ぐ。
            const auto numberKey = static_cast<ImGuiKey>(ImGuiKey_1 + static_cast<int>(i));
            const auto keypadKey = static_cast<ImGuiKey>(ImGuiKey_Keypad1 + static_cast<int>(i));
            if (m_selectionDelay == 0 &&
                (ImGui::IsKeyPressed(numberKey, false) || ImGui::IsKeyPressed(keypadKey, false)))
            {
                selected = static_cast<int>(i);
            }
            ImGui::PopID();
        }
        ImGui::EndDisabled();

        // 描画ループを抜けてから報酬を確定する。
        // Selectで候補配列が更新されるため、走査の途中では呼び出さない。
        if (selected >= 0)
        {
            UpgradeId granted;
            if (m_progression.Select(selected, granted))
            {
                ApplyUpgradeEffects(granted, false);
                m_selectionDelay = 0.2f;

                // まとめて複数レベル上がった場合は閉じず、次の候補を同じ画面で表示する。
                if (!IsChoosingUpgrade())
                {
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }
}
