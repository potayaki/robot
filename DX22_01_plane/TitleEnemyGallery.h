/// @brief ゲームルールからタイトル台座の敵を選ぶ画像ボタン。
#pragma once
#include "TitleEnemyGuide.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <optional>

struct ID3D11ShaderResourceView;

// アトラスはTitleEnemyKind順の正方形5枚。ボタンを押したフレームにその敵を返す。
inline std::optional<TitleEnemyKind> DrawTitleEnemyGallery(ID3D11ShaderResourceView* atlas) {
    constexpr int count = static_cast<int>(TitleEnemyKind::Count);
    const float font = ImGui::GetFontSize();
    const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float gap = spacing.x;
    const float width = (std::max)(1.0f, available.x);
    const float minCardWidth = font * 5.8f;
    int columns = std::clamp(static_cast<int>((width + gap) / (minCardWidth + gap)), 1, count);
    if (columns == 4) {
        columns = 3; // 4枚＋1枚になる幅では3枚＋2枚に整える。
    }
    const float availablePerCard = std::floor((width - gap * (columns - 1)) / columns);
    const float side = std::clamp(availablePerCard, 1.0f, font * 11.5f);
    const int rows = (count + columns - 1) / columns;
    const float gridHeight = rows * side + (rows - 1) * spacing.y;
    if (available.y > gridHeight) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (available.y - gridHeight) * 0.5f);
    }
    const float inset = (std::min)(font * 0.48f, side * 0.08f);
    const ImVec2 imageSize(side - 2 * inset, side - 2 * inset);
    const float left = ImGui::GetCursorPosX();
    const float rounding = font * 0.36f;
    std::optional<TitleEnemyKind> opened;

    ImGui::PushID("TitleEnemyGallery");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(inset, inset));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.16f, 0.20f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.17f, 0.27f, 0.32f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.32f, 0.35f, 1));
    for (int i = 0; i < count; ++i) {
        if (i % columns == 0) {
            const int rowCount = (std::min)(columns, count - i);
            const float rowWidth = rowCount * side + (rowCount - 1) * gap;
            ImGui::SetCursorPosX(left + (width - rowWidth) * 0.5f);
        } else {
            ImGui::SameLine(0, gap);
        }

        const auto kind = static_cast<TitleEnemyKind>(i);
        const char* name = reinterpret_cast<const char*>(GetTitleEnemyInfo(kind).name);
        const bool pressed = atlas ? ImGui::ImageButton(name, reinterpret_cast<ImTextureID>(atlas), imageSize,
                                                        {static_cast<float>(i) / count, 0},
                                                        {static_cast<float>(i + 1) / count, 1})
                                   : ImGui::Button(name, {side, side});
        if (pressed) {
            opened = kind;
        }
        if (ImGui::IsItemVisible()) {
            const bool highlighted =
                ImGui::IsItemHovered() || (ImGui::GetIO().NavVisible && ImGui::IsItemFocused());
            const ImU32 border = highlighted ? IM_COL32(112, 220, 209, 255) : IM_COL32(61, 85, 96, 255);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), border,
                                                rounding, 0, highlighted ? 2.0f : 1.0f);
            ImGui::SetItemTooltip("%s", name);
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return opened;
}
