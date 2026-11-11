/// @file utility.h
/// @brief モデル読込などから使う補助関数。

#pragma once
#include <string>

namespace utility {
    std::string wide_to_multi_winapi(std::wstring const& src);
    std::wstring utf8_to_wide_winapi(std::string const& src);
    std::string utf8_to_multi_winapi(std::string const& src);
}; // namespace utility