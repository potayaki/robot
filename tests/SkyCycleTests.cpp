#include "../DX22_01_plane/SkyCycle.h"
#include <cassert>
#include <iostream>
#include <limits>

using SkyWeather::Cycle;
using SkyWeather::Preset;

int main()
{
    // 曇りは朝と昼の開始時だけ抽選され、夕方と夜には持ち越さない。
    Cycle cycle;
    int rolls = 0;
    auto cloudy = [&rolls] { ++rolls; return 0.05f; };
    cycle.RollWeather(cloudy);
    assert(cycle.Current() == Preset::Overcast && rolls == 1);
    cycle.Update(90.0f, cloudy);
    assert(cycle.Current() == Preset::Evening && rolls == 1);
    cycle.Update(90.0f, cloudy);
    assert(cycle.Current() == Preset::Night && rolls == 1);
    cycle.Update(90.0f, cloudy);
    assert(cycle.TimeOfDay() == Preset::Morning && cycle.Current() == Preset::Overcast && rolls == 2);
    cycle.Update(90.0f, cloudy);
    assert(cycle.TimeOfDay() == Preset::Day && cycle.Current() == Preset::Overcast && rolls == 3);

    // 確率の境界とDebug設定。10%ちょうどは曇りに含めない。
    cycle.RollWeather([] { return 0.10f; });
    assert(cycle.Current() == Preset::Day);
    cycle.cloudyProbability = 0.0f;
    cycle.RollWeather([] { return 0.0f; });
    assert(cycle.Current() == Preset::Day);
    cycle.cloudyProbability = 1.0f;
    cycle.RollWeather([] { return 0.999f; });
    assert(cycle.Current() == Preset::Overcast);

    // 同じ実時間なら30/60/144fpsでも時間帯・抽選回数・進行量が一致する。
    for (int fps : {30, 60, 144})
    {
        Cycle timed;
        int count = 0;
        auto sunny = [&count] { ++count; return 0.5f; };
        for (int frame = 0; frame < fps * 400; ++frame) timed.Update(1.0f / fps, sunny);
        assert(timed.Current() == Preset::Day && count == 2);
        assert(std::abs(timed.Progress() - 40.0f / 90.0f) < 0.0001f);
    }

    // 1回の更新で境界を複数通過しても、通過した朝・昼それぞれを抽選する。
    Cycle catchUp;
    int catchUpRolls = 0;
    catchUp.Update(1092.0f, [&] { ++catchUpRolls; return 0.5f; });
    assert(catchUp.Current() == Preset::Day && catchUpRolls == 6);
    const float progress = catchUp.Progress();
    catchUp.Update(0, cloudy);
    catchUp.Update(-1, cloudy);
    catchUp.Update(std::numeric_limits<float>::quiet_NaN(), cloudy);
    assert(catchUp.Progress() == progress);

    assert(SkyWeather::TransitionWeight(0) == 0);
    assert(SkyWeather::TransitionWeight(4) == 0.5f);
    assert(SkyWeather::TransitionWeight(8) == 1);
    assert(SkyWeather::TransitionWeight(100) == 1);
    std::cout << "Sky cycle tests passed.\n";
}
