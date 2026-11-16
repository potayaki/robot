#pragma once
#include "Texture.h"
#include "TitleEnemyGuide.h"
#include <array>
#include <optional>

struct ImFontGlyphRangesBuilder;

// タイトルのルール画面。タブと見出しを固定し、本文をスクロールする。
class GameRules {
public:
    /// 画像が押されたフレームに、台座で見る敵を返す。
    std::optional<TitleEnemyKind> Draw();
    static void AddGlyphs(ImFontGlyphRangesBuilder& glyphs);

private:
    static constexpr size_t PageCount = 5;
    std::array<Texture, PageCount> m_images{};
    std::array<bool, PageCount> m_triedImage{};
    int m_page = 0;
};
