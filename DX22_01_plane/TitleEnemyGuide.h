/// @brief ルールの画像一覧・解説とタイトル台座が共有する敵の種類。
#pragma once
#include <cstddef>
#include <iterator>

enum class TitleEnemyKind {
    Slime,
    Drone,
    Boar,
    Mushroom,
    Ghost,
    Count
};

struct TitleEnemyInfo {
    const char8_t* name;
    const char8_t* behavior;
    const char8_t* tip;
};

inline const TitleEnemyInfo& GetTitleEnemyInfo(TitleEnemyKind kind) {
    static constexpr TitleEnemyInfo enemies[] = {
        {u8"スライム",
         u8"地上を追いかけて、体当たりしてくる。小型は時々ダッシュし、大型は足が遅いぶん体力が高い。",
         u8"小型に追いつかれる前に数を減らそう。大型とは距離を保つと、体当たりを受けずに狙える。"},
        {u8"ドローン", u8"上空で距離を取り、間合いに入ると機体をめがけて急降下する。",
         u8"高度が下がり始めたら横へ移動しよう。すれ違った後は、再び上昇する機体を照準で追える。"},
        {u8"イノシシ", u8"地面の予告線に沿って、一直線に突進する。走り切ると一度立ち止まる。",
         u8"予告線が出たら横に抜けよう。振り向いて、突進直後の隙を狙おう。"},
        {u8"自爆キノコ", u8"跳ねながら近づき、足元で立ち止まる。赤く点滅した後、周囲を巻き込んで爆発する。",
         u8"点滅が始まったら、その場から離れよう。複数のキノコがいるときは、別の爆発に飛び込まないように。"},
        {u8"ゴースト", u8"光の線でつないだ敵の攻撃力と防御力を高める。支援する仲間がいないときは霊弾を放つ。",
         u8"強化された敵がいたら、線をたどってゴーストを先に狙おう。単独のときは霊弾を横移動でかわせる。"},
    };
    static_assert(std::size(enemies) == static_cast<std::size_t>(TitleEnemyKind::Count));
    return enemies[static_cast<std::size_t>(kind)];
}
