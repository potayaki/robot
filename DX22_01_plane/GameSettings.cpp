/// @brief タイトル設定の保存、表示オプション、ゲームの効果音。
#include "GameSettings.h"
#include "Application.h"
#include "Renderer.h"
#include "imgui.h"
#include <ShlObj.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <xaudio2.h>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <wrl/client.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
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

    // LAMEのInfo/Xingタグに記録された元の長さを使い、圧縮時の余白だけを除く。
    void TrimMp3Padding(const wchar_t* path, size_t blockAlign, std::vector<BYTE>& pcm) {
        std::ifstream input(std::filesystem::path(path), std::ios::binary);
        std::array<BYTE, 10> id3{};
        if (!input.read(reinterpret_cast<char*>(id3.data()), id3.size()) || !blockAlign) {
            return;
        }
        std::streamoff frameOffset = 0;
        if (std::memcmp(id3.data(), "ID3", 3) == 0) {
            for (size_t i = 6; i < id3.size(); ++i) {
                frameOffset = (frameOffset << 7) | (id3[i] & 0x7f);
            }
            frameOffset += id3.size();
            if (id3[3] == 4 && (id3[5] & 0x10)) {
                frameOffset += 10; // ID3v2.4の任意フッター
            }
        }
        input.seekg(frameOffset);
        std::array<BYTE, 192> header{};
        if (!input.read(reinterpret_cast<char*>(header.data()), header.size()) || header[0] != 0xff ||
            (header[1] & 0xe6) != 0xe2) {
            return;
        }
        const bool mpeg1 = (header[1] & 0x18) == 0x18;
        const bool mono = (header[3] & 0xc0) == 0xc0;
        const size_t sideInfo = mpeg1 ? (mono ? 17 : 32) : (mono ? 9 : 17);
        const BYTE* xing = header.data() + 4 + sideInfo;
        if ((std::memcmp(xing, "Info", 4) && std::memcmp(xing, "Xing", 4)) || !(xing[7] & 1)) {
            return;
        }
        const uint32_t frameCount =
            (uint32_t{xing[8]} << 24) | (uint32_t{xing[9]} << 16) | (uint32_t{xing[10]} << 8) | xing[11];
        // 可変長のXingフィールド（ファイル長・シーク表・品質）の次がLAMEタグ。
        const BYTE* lame =
            xing + 12 + ((xing[7] & 2) ? 4 : 0) + ((xing[7] & 4) ? 100 : 0) + ((xing[7] & 8) ? 4 : 0);
        if (std::memcmp(lame, "LAME", 4) != 0) {
            return;
        }
        const size_t delay = (lame[21] << 4) | (lame[22] >> 4);
        const size_t padding = ((lame[22] & 0x0f) << 8) | lame[23];
        const size_t samplesPerFrame = mpeg1 ? 1152 : 576;
        const uint64_t encoded = uint64_t{frameCount} * samplesPerFrame;
        constexpr size_t decoderDelay = 529;
        const size_t decoded = pcm.size() / blockAlign;
        // Windowsのデコーダーがタグの1フレームを含め、既に528サンプルを省いた場合に限る。
        if (encoded + samplesPerFrame != decoded + decoderDelay - 1 || encoded <= delay + padding ||
            padding < decoderDelay) {
            return;
        }
        const size_t begin = samplesPerFrame + delay + 1;
        const size_t length = static_cast<size_t>(encoded - delay - padding);
        pcm.resize((begin + length) * blockAlign);
        pcm.erase(pcm.begin(), pcm.begin() + begin * blockAlign);
    }

    // 短い効果音は起動時にPCMへ展開し、戦闘中はファイルを開かずに再生する。
    bool ReadPcmSamples(IMFSourceReader* reader, std::vector<BYTE>& pcm) {
        DWORD flags = 0;
        do {
            Microsoft::WRL::ComPtr<IMFSample> sample;
            if (FAILED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr,
                                          sample.GetAddressOf())) ||
                (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))) {
                return false;
            }
            if (!sample) {
                continue;
            }
            Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
            if (FAILED(sample->ConvertToContiguousBuffer(buffer.GetAddressOf()))) {
                return false;
            }
            BYTE* data = nullptr;
            DWORD length = 0;
            if (FAILED(buffer->Lock(&data, nullptr, &length))) {
                return false;
            }
            const bool fits = length <= XAUDIO2_MAX_BUFFER_BYTES - pcm.size();
            if (fits && length > 0) {
                pcm.insert(pcm.end(), data, data + length);
            }
            buffer->Unlock();
            if (!fits) {
                return false;
            }
        } while (!(flags & MF_SOURCE_READERF_ENDOFSTREAM));
        return !pcm.empty();
    }

    struct GameAudio {
        enum Cue {
            Menu,
            Shot,
            Impact,
            Defeat,
            Experience,
            StartClick,
            StartShot,
            Count
        };

        std::array<std::vector<BYTE>, Count> samples{};
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

        bool LoadCue(Cue cue, const wchar_t* path) {
            using Microsoft::WRL::ComPtr;
            ComPtr<IMFSourceReader> reader;
            ComPtr<IMFMediaType> request;
            ComPtr<IMFMediaType> output;
            constexpr DWORD stream = MF_SOURCE_READER_FIRST_AUDIO_STREAM;
            if (FAILED(MFCreateSourceReaderFromURL(path, nullptr, reader.GetAddressOf())) ||
                FAILED(MFCreateMediaType(request.GetAddressOf())) ||
                FAILED(request->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
                FAILED(request->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
                FAILED(reader->SetCurrentMediaType(stream, nullptr, request.Get())) ||
                FAILED(reader->GetCurrentMediaType(stream, output.GetAddressOf()))) {
                return false;
            }

            std::vector<BYTE> pcm;
            if (!ReadPcmSamples(reader.Get(), pcm)) {
                return false;
            }
            WAVEFORMATEX* format = nullptr;
            UINT32 formatSize = 0;
            if (FAILED(MFCreateWaveFormatExFromMFMediaType(output.Get(), &format, &formatSize))) {
                return false;
            }
            TrimMp3Padding(path, format->nBlockAlign, pcm);
            const HRESULT result = engine->CreateSourceVoice(&voices[cue], format);
            if (SUCCEEDED(result)) {
                samples[cue].swap(pcm);
                if (cue == Shot) {
                    engine->CreateSourceVoice(&alternateShot, format);
                }
            }
            CoTaskMemFree(format);
            return SUCCEEDED(result);
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
            if (FAILED(MFStartup(MF_VERSION))) {
                return;
            }
            constexpr std::array paths{L"assets/audio/menu.mp3",       L"assets/audio/shot.mp3",
                                       L"assets/audio/impact.mp3",     L"assets/audio/defeat.mp3",
                                       L"assets/audio/experience.mp3", L"assets/audio/start_click.mp3",
                                       L"assets/audio/start_shot.mp3"};
            static_assert(paths.size() == Count);
            for (size_t i = 0; i < paths.size(); ++i) {
                if (!LoadCue(static_cast<Cue>(i), paths[i])) {
                    OutputDebugStringW(L"Could not load sound effect: ");
                    OutputDebugStringW(paths[i]);
                    OutputDebugStringW(L"\n");
                }
            }
            MFShutdown();
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
            buffer.pAudioData = samples[cue].data();
            buffer.AudioBytes = static_cast<UINT32>(samples[cue].size());
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

void GameSettings::PlayExperienceSound() {
    audio.Play(GameAudio::Experience);
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
