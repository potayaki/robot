#include "GameRules.h"
#include "imgui.h"
#include "SurvivorProgression.h"
#include "TitleEnemyGallery.h"
#include <algorithm>
#include <cmath>
#include <span>

namespace {
    const char* Text(const char8_t* text) {
        return reinterpret_cast<const char*>(text);
    }

    struct Entry {
        const char8_t* label;
        const char8_t* detail;
    };

    enum class PageContent {
        Guide,
        Skills,
        EnemyGallery
    };

    struct Page {
        const char8_t* tab;
        const char8_t* title;
        const char* image;
        std::span<const Entry> entries;
        PageContent content = PageContent::Guide;
    };

    constexpr Entry Controls[] = {
        {u8"移動", u8"W / A / S / D で前後左右へ。Space でジャンプ。"},
        {u8"照準", u8"マウスで敵を狙う。カーソルを画面の左右端に寄せると視点が回る。"},
        {u8"通常弾", u8"左クリックを押し続けると、照準に向かって連射する。"},
        {u8"ミサイル", u8"大きい照準円に敵を入れて右クリック。青いリングが満ちると再発射できる。"},
        {u8"スキル確認", u8"Tab で戦闘を一時停止。装備中の効果とレベルを読み、もう一度 Tab で戻る。"},
    };
    constexpr Entry HUD[] = {
        {u8"左上の地図", u8"中央が自機、赤い点が敵。背後や側面からの接近も確認できる。"},
        {u8"左下のリング", u8"外側の赤は残りHP。内側の青はミサイルの再装填。"},
        {u8"上部の時計", u8"生存時間を示す。下の KILLS は撃破数、EXP は次のレベルまでの経験値。"},
        {u8"右側の装備一覧",
         u8"取得したスキルとレベルを表示。「効果の詳細を見る」か Tab で、威力や発動間隔を確認できる。"},
        {u8"レベルアップ",
         u8"敵を倒すと経験値が増える。候補をクリックするか、数字キー 1～3 で強化を一つ選ぶ。"},
        {u8"補給", u8"プレゼントに近づくと武器やスキルを入手する。スパナはHPを回復する。"},
    };
    constexpr Entry Skills[] = {
        {u8"レベルアップとプレゼント",
         u8"レベルアップでは候補から1つ選び、プレゼントではランダムに1つ入手する。"
         u8"同じスキルを取るとレベルが上がる。戦闘中はTabで装備中の効果を確認できる。"},
    };

    struct SkillGroup {
        const char8_t* name;
        std::span<const UpgradeId> skills;
    };

    constexpr UpgradeId BulletSkills[] = {UpgradeId::BulletPower, UpgradeId::RapidFire, UpgradeId::MultiShot,
                                          UpgradeId::BulletHoming};
    constexpr UpgradeId MissileSkills[] = {UpgradeId::MissilePower, UpgradeId::MissileVolley,
                                           UpgradeId::MissileReload};
    constexpr UpgradeId AutoSkills[] = {UpgradeId::Orbit, UpgradeId::Turret, UpgradeId::Shockwave,
                                        UpgradeId::Lightning};
    constexpr UpgradeId BodySkills[] = {UpgradeId::MoveSpeed, UpgradeId::Vitality, UpgradeId::Armor,
                                        UpgradeId::Regeneration, UpgradeId::Experience};
    constexpr SkillGroup SkillGroups[] = {
        {u8"通常弾", BulletSkills},
        {u8"ミサイル", MissileSkills},
        {u8"自動武器", AutoSkills},
        {u8"機体・成長", BodySkills},
    };
    static_assert(std::size(BulletSkills) + std::size(MissileSkills) + std::size(AutoSkills) +
                      std::size(BodySkills) ==
                  static_cast<size_t>(UpgradeId::Count));
    constexpr auto SkillLabel = u8"%s（最大Lv.%d）";
    constexpr auto SkillHelp = u8"スキル名をクリックすると、各レベルの性能を確認できる。";
    constexpr Entry Goal[] = {
        {u8"ボスの出現", u8"通常の敵を30体倒すと、バリアの中でスライムキングとの一騎打ちが始まる。"},
        {u8"突進のかわし方", u8"ボスが構えたら横へ。突進のあとの休息が攻撃のチャンス。"},
        {u8"ボス戦のクリア条件", u8"スライムキングのHPを0にすると勝利。バリアが消えて通常戦に戻り、さらに30体"
                                 u8"倒すと次のボス戦が始まる。"},
        {u8"プレイの目標", u8"生存時間を伸ばすエンドレス戦。HPが0になると終了し、生存時間・撃破数・到達レベル"
                           u8"・倒したボスの種類数が結果に残る。"},
    };
    constexpr Page Pages[] = {
        {u8"基本操作", u8"移動しながら狙いを合わせる", "assets/texture/rules/controls.png", Controls},
        {u8"画面の見方", u8"機体の状態と周囲を確認する", "assets/texture/rules/hud.png", HUD},
        {u8"スキル", u8"スキルの入手と強化", "assets/texture/rules/skills.png", Skills, PageContent::Skills},
        {u8"ボスと目標", u8"ボスを突破して生き延びる", "assets/texture/rules/victory.png", Goal},
        {u8"敵の特徴",
         u8"画像を選んでフィギュアを見る",
         "assets/texture/rules/enemy_portraits.png",
         {},
         PageContent::EnemyGallery},
    };
    constexpr ImVec4 Gold(0.96f, 0.78f, 0.48f, 1.0f);

    void DrawSkillList() {
        ImGui::TextWrapped("%s", Text(SkillHelp));
        if (!ImGui::BeginTabBar("SkillGroups")) {
            return;
        }
        for (const auto& group : SkillGroups) {
            if (!ImGui::BeginTabItem(Text(group.name))) {
                continue;
            }
            for (const auto id : group.skills) {
                const auto& skill = GetUpgradeDefinition(id);
                ImGui::PushID(static_cast<int>(id));
                ImGui::PushStyleColor(ImGuiCol_Text, Gold);
                const bool showRanks = ImGui::TreeNodeEx(
                    "ranks", ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
                    Text(SkillLabel), Text(skill.name), skill.maxRank);
                ImGui::PopStyleColor();
                ImGui::TextWrapped("%s", Text(skill.description));
                if (showRanks) {
                    ImGui::Indent();
                    for (int rank = 1; rank <= skill.maxRank; ++rank) {
                        const auto effect = GetUpgradeEffectText(id, rank);
                        ImGui::TextWrapped("Lv.%d  %s", rank, effect.c_str());
                    }
                    ImGui::Unindent();
                }
                ImGui::Spacing();
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // 同じ行の見出しと説明の高さをページ間でそろえる。折り返し量は現在の列幅から求める。
    ImVec2 RowTextHeights(size_t row, float width) {
        ImVec2 heights(0, 0);
        for (const auto& page : Pages) {
            if (page.content == PageContent::Guide && row < page.entries.size()) {
                const auto& entry = page.entries[row];
                heights.x =
                    (std::max)(heights.x, ImGui::CalcTextSize(Text(entry.label), nullptr, false, width).y);
                heights.y =
                    (std::max)(heights.y, ImGui::CalcTextSize(Text(entry.detail), nullptr, false, width).y);
            }
        }
        return heights;
    }

    void DrawEntries(const Page& page) {
        const float width = (std::max)(1.0f, ImGui::GetContentRegionAvail().x);
        const float gap = ImGui::GetStyle().ItemSpacing.y;
        ImGui::PushTextWrapPos(0);
        for (size_t row = 0; row < page.entries.size(); ++row) {
            const auto& entry = page.entries[row];
            ImGui::PushStyleColor(ImGuiCol_Text, Gold);
            const float rowTop = ImGui::GetCursorPosY();
            ImGui::TextUnformatted(Text(entry.label));
            ImGui::PopStyleColor();
            const ImVec2 heights = RowTextHeights(row, width);
            ImGui::SetCursorPosY((std::max)(ImGui::GetCursorPosY(), rowTop + heights.x + gap));
            ImGui::TextUnformatted(Text(entry.detail));
            if (row + 1 < page.entries.size()) {
                ImGui::SetCursorPosY(rowTop + heights.x + heights.y + 3 * gap);
            }
        }
        ImGui::PopTextWrapPos();
        if (page.content == PageContent::Skills) {
            ImGui::Spacing();
            DrawSkillList();
        }
    }

    void DrawImage(ID3D11ShaderResourceView* view, float width) {
        const ImVec2 size(width, width * 9.0f / 16.0f);
        if (view) {
            ImGui::Image(reinterpret_cast<ImTextureID>(view), size);
        } else {
            ImGui::Dummy(size);
        }
    }
} // namespace

void GameRules::AddGlyphs(ImFontGlyphRangesBuilder& glyphs) {
    glyphs.AddText(Text(SkillLabel));
    glyphs.AddText(Text(SkillHelp));
    glyphs.AddText(Text(u8"行動の特徴 対処のコツ 一覧に戻る 前の敵 次の敵"));
    for (int i = 0; i < static_cast<int>(TitleEnemyKind::Count); ++i) {
        const auto& enemy = GetTitleEnemyInfo(static_cast<TitleEnemyKind>(i));
        glyphs.AddText(Text(enemy.name));
        glyphs.AddText(Text(enemy.behavior));
        glyphs.AddText(Text(enemy.tip));
    }
    for (const auto& group : SkillGroups) {
        glyphs.AddText(Text(group.name));
    }
    for (const auto& page : Pages) {
        glyphs.AddText(Text(page.tab));
        glyphs.AddText(Text(page.title));
        for (const auto& entry : page.entries) {
            glyphs.AddText(Text(entry.label));
            glyphs.AddText(Text(entry.detail));
        }
    }
}

std::optional<TitleEnemyKind> GameRules::Draw() {
    static_assert(PageCount == std::size(Pages));
    ImGui::PushID(this);
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float width = ImGui::GetContentRegionAvail().x;
    float minWidth = 0;
    float titleHeight = 0;
    for (const auto& page : Pages) {
        minWidth = (std::max)(minWidth, ImGui::CalcTextSize(Text(page.tab)).x);
        titleHeight = (std::max)(titleHeight, ImGui::CalcTextSize(Text(page.title), nullptr, false, width).y);
    }
    minWidth += 2 * ImGui::GetStyle().FramePadding.x;
    const int columns =
        std::clamp(static_cast<int>((width + gap) / (minWidth + gap)), 1, static_cast<int>(PageCount));
    const float tabWidth = (std::max)(1.0f, (width - gap * (columns - 1)) / columns);
    const int previousPage = m_page;
    for (int i = 0; i < static_cast<int>(std::size(Pages)); ++i) {
        if (i % columns) {
            ImGui::SameLine();
        }
        const bool active = m_page == i;
        ImGui::PushStyleColor(ImGuiCol_Button,
                              active ? ImVec4(0.42f, 0.30f, 0.16f, 1) : ImVec4(0.14f, 0.18f, 0.22f, 1));
        if (ImGui::Button(Text(Pages[i].tab), {tabWidth, ImGui::GetFrameHeight() * 1.2f})) {
            m_page = i;
        }
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();
    const auto& page = Pages[m_page];
    const float titleTop = ImGui::GetCursorPosY();
    ImGui::TextWrapped("%s", Text(page.title));
    ImGui::SetCursorPosY(titleTop + titleHeight + ImGui::GetStyle().ItemSpacing.y);
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_triedImage[m_page]) {
        m_triedImage[m_page] = true;
        m_images[m_page].Load(page.image);
    }
    auto view = m_images[m_page].GetShaderResourceView();
    if (page.content == PageContent::EnemyGallery) {
        std::optional<TitleEnemyKind> opened;
        if (ImGui::BeginChild("RulesEnemyGallery")) {
            opened = DrawTitleEnemyGallery(view);
        }
        ImGui::EndChild();
        ImGui::PopID();
        return opened;
    }
    // タブの下に左列を確保し、右列のスクロール量から独立させる。
    const bool sideBySide = width >= ImGui::GetFontSize() * 36.0f;
    if (sideBySide) {
        const float columnWidth = std::floor((width - gap) * 0.52f);
        const float textX = ImGui::GetCursorPosX() + columnWidth + gap;
        const float availableHeight = (std::max)(1.0f, ImGui::GetContentRegionAvail().y);
        DrawImage(view, (std::min)(columnWidth, availableHeight * 16.0f / 9.0f));
        ImGui::SameLine(textX);
    }
    if (previousPage != m_page) {
        ImGui::SetNextWindowScroll(ImVec2(0, 0));
    }
    if (ImGui::BeginChild("RulesReading", ImVec2(0, 0), ImGuiChildFlags_None,
                          ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        if (!sideBySide) {
            const float imageWidth =
                (std::min)(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 26.0f);
            DrawImage(view, imageWidth);
            ImGui::Spacing();
        }
        DrawEntries(page);
    }
    ImGui::EndChild();
    ImGui::PopID();
    return std::nullopt;
}
