/// @brief 結果の表示と、タイトル・再挑戦のクリック操作。
#include "ResultScene.h"
#include "Game.h"
#include "imgui.h"
#include <algorithm>

void ResultScene::DrawUI() {
    const auto text = [](const char8_t* value) { return reinterpret_cast<const char*>(value); };
    const auto size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowBgAlpha(1.0f);
    constexpr auto flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("ResultScreen", nullptr, flags);
    const float width = (std::min)(420.0f, size.x - 32.0f);
    ImGui::SetCursorPos(ImVec2((size.x - width) * 0.5f, (std::max)(16.0f, (size.y - 350.0f) * 0.5f)));
    ImGui::BeginChild("ResultContents", ImVec2(width, 350), false);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.35f, 1.0f), "%s", text(u8"ゲームオーバー"));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    const int seconds = static_cast<int>(m_result.survivalSeconds);
    ImGui::Text(text(u8"生存時間：%02d:%02d"), seconds / 60, seconds % 60);
    ImGui::Text(text(u8"撃破数：%d"), m_result.kills);
    ImGui::Text(text(u8"到達レベル：%d"), m_result.level);
    ImGui::Text(text(u8"ボス撃破：%d / %d 種類"), m_result.defeatedBossTypes, RunResult::TotalBossTypes);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button(text(u8"もう一度遊ぶ"), ImVec2(-1, 52))) {
        Game::GetInstance()->ChangeScene(STAGE1);
    }
    ImGui::Spacing();
    if (ImGui::Button(text(u8"タイトル"), ImVec2(-1, 52))) {
        Game::GetInstance()->ChangeScene(TITLE);
    }
    ImGui::EndChild();
    ImGui::End();
}
