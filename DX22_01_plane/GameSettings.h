#pragma once

// ユーザーごとの設定。描画・入力側にも、画面に表示する値と同じ値を渡す。
namespace GameSettings {
    void Load();
    void Shutdown();
    void DrawControls();
    void PlayMenuSound();
    void PlayShotSound();
    void PlayImpactSound();
    void PlayDefeatSound();
    void PlayExperienceSound();
    void PlayExplosionSound();
    void PlayStartSound();
    void PlayStartShotSound();
    bool VSync();
    bool Msaa4x();
    int FrameLimit();
    bool Fullscreen();
    void SetFullscreen(bool enabled);
} // namespace GameSettings
