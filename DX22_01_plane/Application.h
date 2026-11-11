/// @file Application.h
/// @brief Windowsウィンドウの作成、メインループ、終了処理。

#pragma once

#include <Windows.h>
#include <cstdint>

//-----------------------------------------------------------------------------
// Applicationクラス
//-----------------------------------------------------------------------------
class Application {
public:
    Application(uint32_t width, uint32_t height);
    ~Application();
    void Run();

    // 幅を取得
    static uint32_t GetWidth() {
        return m_width;
    }

    // 高さを取得
    static uint32_t GetHeight() {
        return m_height;
    }

    // ウインドウハンドルを返す
    static HWND GetWindow() {
        return m_hWnd;
    }

private:
    /// インスタンスハンドル
    static HINSTANCE m_hInst;
    /// ウィンドウハンドル
    static HWND m_hWnd;
    /// ウィンドウの横幅
    static uint32_t m_width;
    /// ウィンドウの縦幅
    static uint32_t m_height;

    static bool InitApp(); //初期化
    static void UninitApp(); //終了処理
    static void MainLoop(); //メインループ

    //プロシージャ
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};
