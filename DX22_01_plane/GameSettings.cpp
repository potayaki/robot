/// @brief タイトル設定の保存、表示オプション、ゲームの効果音。
#include "GameSettings.h"
#include "Application.h"
#include "Renderer.h"
#include "imgui.h"
#include <ShlObj.h>
#include <xaudio2.h>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <wrl/client.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace {
    struct Options {
        int volume = 70;
        int frameLimit = 60;
        bool vsync = true;
        bool msaa4x = true;
        bool fullscreen = true;
    };

    Options options;
    std::filesystem::path file;
    bool dirty = false;
    bool saveError = false;

    std::filesystem::path SettingsPath() {
        PWSTR folder = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder))) {
            return {};
        }
        std::filesystem::path path = std::filesystem::path(folder) / L"RobotSurvivor" / L"settings.ini";
        CoTaskMemFree(folder);
        return path;
    }

    void ReadSettings() {
        std::ifstream input(file);
        for (std::string line; std::getline(input, line);) {
            const size_t equals = line.find('=');
            if (equals == std::string::npos) {
                continue;
            }
            const std::string_view name(line.data(), equals);
            const char* last = line.data() + line.size();
            int value = 0;
            const auto result = std::from_chars(line.data() + equals + 1, last, value);
            if (result.ec != std::errc{} || result.ptr != last) {
                continue;
            }
            if (name == "volume" && value >= 0 && value <= 100) {
                options.volume = value;
            }
            if (name == "frameLimit" && (value == 30 || value == 60 || value == 120)) {
                options.frameLimit = value;
            }
            if (name == "vsync" && (value == 0 || value == 1)) {
                options.vsync = value != 0;
            }
            if (name == "msaa4x" && (value == 0 || value == 1)) {
                options.msaa4x = value != 0;
            }
            if (name == "fullscreen" && (value == 0 || value == 1)) {
                options.fullscreen = value != 0;
            }
        }
    }

    void SaveSettings() {
        if (!dirty) {
            return;
        }
        saveError = true;
        if (file.empty()) {
            return;
        }
        std::error_code error;
        std::filesystem::create_directories(file.parent_path(), error);
        if (error) {
            return;
        }
        // 異常終了時に以前の設定を失わないよう、書き終えたファイルを一度で置き換える。
        auto temp = file;
        temp += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
        std::ofstream out(temp, std::ios::trunc);
        out << "volume=" << options.volume << "\nframeLimit=" << options.frameLimit
            << "\nvsync=" << options.vsync << "\nfullscreen=" << options.fullscreen
            << "\nmsaa4x=" << options.msaa4x << '\n';
        out.close();
        if (!out ||
            !MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::filesystem::remove(temp, error);
            return;
        }
        dirty = saveError = false;
    }

    // 短い電子音を起動時に一度作る。音声出力が使えない場合もプレイは続けられる。
    struct GameAudio {
        static constexpr unsigned SampleRate = 24000;

        enum Cue {
            Menu,
            Shot,
            Impact,
            Defeat,
            StartClick,
            StartShot,
            Count
        };

        std::array<std::vector<float>, Count> samples{};
        Microsoft::WRL::ComPtr<IXAudio2> engine;
        IXAudio2MasteringVoice* master = nullptr;
        std::array<IXAudio2SourceVoice*, Count> voices{};
        /// 高速装填でも前の発砲音の余韻を重ねられる予備ボイス。
        IXAudio2SourceVoice* alternateShot = nullptr;
        /// 効果音の音程専用。敵や報酬の抽選には使わない。
        uint32_t pitchState = 1993;
        bool ownsCom = false;

        void ApplyVolume() {
            const float level = static_cast<float>(options.volume) / 100;
            if (master) {
                master->SetVolume(level * level);
            }
        }

        void Init() {
            const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            ownsCom = SUCCEEDED(com);
            if (!ownsCom && com != RPC_E_CHANGED_MODE) {
                return;
            }
            if (FAILED(XAudio2Create(engine.GetAddressOf())) ||
                FAILED(engine->CreateMasteringVoice(&master))) {
                return;
            }
            WAVEFORMATEX format{};
            format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            format.nChannels = 1;
            format.nSamplesPerSec = SampleRate;
            format.wBitsPerSample = 32;
            format.nBlockAlign = sizeof(float);
            format.nAvgBytesPerSec = SampleRate * format.nBlockAlign;
            constexpr double TwoPi = 6.283185307179586;
            samples[Menu].resize(2400);
            for (unsigned i = 0; i < samples[Menu].size(); ++i) {
                const double t = static_cast<double>(i) / SampleRate;
                const double fade = (1.0 - std::exp(-t * 950.0)) *
                                    (1.0 - static_cast<double>(i) / (samples[Menu].size() - 1));
                samples[Menu][i] =
                    static_cast<float>(0.20 * fade * std::exp(-t * 22) *
                                       (std::sin(TwoPi * 660 * t) + 0.24 * std::sin(TwoPi * 990 * t)));
            }
            // 発射は低い機械音、命中は短い金属音、撃破には下降する低音と明るい余韻を重ねる。
            samples[Shot].resize(SampleRate * 75 / 1000);
            samples[Impact].resize(SampleRate * 55 / 1000);
            samples[Defeat].resize(SampleRate * 18 / 100);
            uint32_t grain = 719u;
            for (size_t i = 0; i < samples[Defeat].size(); ++i) {
                const double t = static_cast<double>(i) / SampleRate;
                const double attack = 1.0 - std::exp(-t * 2200.0);
                grain = grain * 1664525u + 1013904223u;
                const double noiseSample = 2.0 * grain / UINT32_MAX - 1.0;
                if (i < samples[Shot].size()) {
                    const double body = std::sin(TwoPi * (90.0 * t + 1.2 * (1.0 - std::exp(-t * 75.0))));
                    samples[Shot][i] =
                        static_cast<float>(attack * (0.26 * std::exp(-t * 65.0) * body +
                                                     0.22 * std::exp(-t * 104.0) * noiseSample));
                }
                if (i < samples[Impact].size()) {
                    const double tick = std::sin(TwoPi * 1180.0 * t) + 0.5 * std::sin(TwoPi * 2130.0 * t);
                    samples[Impact][i] = static_cast<float>(attack * std::exp(-t * 92.0) * 0.21 * tick);
                }
                const double drop = std::sin(TwoPi * (64.0 * t + 3.0 * (1.0 - std::exp(-t * 30.0))));
                const double ring = std::sin(TwoPi * 590.0 * t) + 0.55 * std::sin(TwoPi * 885.0 * t);
                samples[Defeat][i] = static_cast<float>(
                    attack * (0.34 * std::exp(-t * 26.0) * drop + 0.17 * std::exp(-t * 35.0) * ring));
            }
            // 展示が起動する短い金属音と、低音が少し残る発砲音。
            samples[StartClick].resize(SampleRate * 6 / 100);
            for (size_t i = 0; i < samples[StartClick].size(); ++i) {
                const double t = static_cast<double>(i) / SampleRate;
                const double attack = 1.0 - std::exp(-t * 2800.0);
                samples[StartClick][i] = static_cast<float>(
                    attack * std::exp(-t * 125.0) *
                    (0.26 * std::sin(TwoPi * 1560 * t) + 0.16 * std::sin(TwoPi * 2980 * t)));
            }
            samples[StartShot].resize(SampleRate * 36 / 100);
            uint32_t noise = 771u;
            for (size_t i = 0; i < samples[StartShot].size(); ++i) {
                const double t = static_cast<double>(i) / SampleRate;
                noise = noise * 1664525u + 1013904223u;
                const double crack = (2.0 * noise / UINT32_MAX - 1.0) * std::exp(-t * 56.0);
                const double body = std::sin(TwoPi * (82.0 * t + 2.5 * (1.0 - std::exp(-t * 50.0))));
                samples[StartShot][i] = static_cast<float>(
                    (1.0 - std::exp(-t * 1700.0)) * (0.42 * crack + 0.44 * std::exp(-t * 19.0) * body));
            }
            for (auto& voice : voices) {
                engine->CreateSourceVoice(&voice, &format);
            }
            engine->CreateSourceVoice(&alternateShot, &format);
            ApplyVolume();
        }

        void Play(Cue cue) {
            auto* voice = voices[cue];
            if (!voice || options.volume == 0) {
                return;
            }
            XAUDIO2_VOICE_STATE state{};
            voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued && cue == Shot && alternateShot) {
                voice = alternateShot;
                voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            }
            if (state.BuffersQueued) {
                return;
            }
            if (cue == Shot || cue == Impact || cue == Defeat) {
                pitchState = pitchState * 1664525u + 1013904223u;
                voice->SetFrequencyRatio(0.94f + 0.12f * static_cast<float>(pitchState >> 16) / 65535.0f);
            }
            XAUDIO2_BUFFER buffer{};
            buffer.pAudioData = reinterpret_cast<const BYTE*>(samples[cue].data());
            buffer.AudioBytes = static_cast<UINT32>(samples[cue].size() * sizeof(float));
            buffer.Flags = XAUDIO2_END_OF_STREAM;
            if (SUCCEEDED(voice->SubmitSourceBuffer(&buffer))) {
                voice->Start();
            }
        }

        void Shutdown() {
            if (alternateShot) {
                alternateShot->DestroyVoice();
                alternateShot = nullptr;
            }
            for (auto& voice : voices) {
                if (voice) {
                    voice->DestroyVoice();
                }
                voice = nullptr;
            }
            if (master) {
                master->DestroyVoice();
            }
            master = nullptr;
            engine.Reset();
            if (ownsCom) {
                CoUninitialize();
            }
            ownsCom = false;
        }
    } audio;

    const char* Text(const char8_t* text) {
        return reinterpret_cast<const char*>(text);
    }
} // namespace

void GameSettings::Load() {
    options = Options{};
    file = SettingsPath();
    if (!file.empty()) {
        ReadSettings();
    }
    audio.Init();
}

void GameSettings::Shutdown() {
    SaveSettings();
    audio.Shutdown();
}

void GameSettings::PlayMenuSound() {
    audio.Play(GameAudio::Menu);
}

void GameSettings::PlayShotSound() {
    audio.Play(GameAudio::Shot);
}

void GameSettings::PlayImpactSound() {
    audio.Play(GameAudio::Impact);
}

void GameSettings::PlayDefeatSound() {
    audio.Play(GameAudio::Defeat);
}

void GameSettings::PlayExplosionSound() {
    audio.Play(GameAudio::StartShot);
}

void GameSettings::PlayStartSound() {
    audio.Play(GameAudio::StartClick);
}

void GameSettings::PlayStartShotSound() {
    audio.Play(GameAudio::StartShot);
}

bool GameSettings::VSync() {
    return options.vsync;
}

bool GameSettings::Msaa4x() {
    return options.msaa4x;
}

int GameSettings::FrameLimit() {
    return options.frameLimit;
}

bool GameSettings::Fullscreen() {
    return options.fullscreen;
}

void GameSettings::SetFullscreen(bool enabled) {
    if (options.fullscreen == enabled) {
        return;
    }
    options.fullscreen = enabled;
    Application::ApplyDisplayMode(enabled);
    dirty = true;
    SaveSettings();
}

void GameSettings::DrawControls() {
    ImGui::SeparatorText(Text(u8"サウンド"));
    ImGui::TextUnformatted(Text(u8"音量"));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##volume", &options.volume, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
        audio.ApplyVolume();
        dirty = true;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        SaveSettings();
        PlayMenuSound();
    }
    ImGui::BeginDisabled(!audio.voices[GameAudio::Menu]);
    if (ImGui::Button(Text(u8"音を確認"))) {
        PlayMenuSound();
    }
    ImGui::EndDisabled();
    if (!audio.voices[GameAudio::Menu]) {
        ImGui::TextDisabled("%s", Text(u8"音声出力を利用できません"));
    }

    ImGui::Spacing();
    ImGui::SeparatorText(Text(u8"グラフィック"));
    bool full = options.fullscreen;
    if (ImGui::Checkbox(Text(u8"フルスクリーン (F11)"), &full)) {
        SetFullscreen(full);
    }
    if (ImGui::Checkbox(Text(u8"垂直同期"), &options.vsync)) {
        dirty = true;
        SaveSettings();
    }
    const bool supported = Renderer::SupportsMsaa4x();
    bool msaa = supported && options.msaa4x;
    ImGui::BeginDisabled(!supported);
    if (ImGui::Checkbox(Text(u8"アンチエイリアス (4x MSAA)"), &msaa)) {
        options.msaa4x = msaa;
        dirty = true;
        SaveSettings();
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", Text(supported ? u8"次回起動時に反映" : u8"この GPU は 4x MSAA 非対応です"));
    ImGui::TextUnformatted(Text(u8"最大フレームレート"));
    constexpr int limits[] = {30, 60, 120};
    const char* labels[] = {"30 FPS", "60 FPS", "120 FPS"};
    int index = options.frameLimit == 30 ? 0 : options.frameLimit == 120 ? 2 : 1;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##fps", &index, labels, IM_ARRAYSIZE(labels))) {
        options.frameLimit = limits[index];
        dirty = true;
        SaveSettings();
    }
    if (saveError) {
        ImGui::TextWrapped("%s", Text(u8"設定を保存できませんでした。"));
    }
}
