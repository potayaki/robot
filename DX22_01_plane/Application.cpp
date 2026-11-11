/// @file Application.cpp
/// @brief Windowsウィンドウの作成、メインループ、終了処理。

#include <chrono>
#include <thread>
#include "Application.h"
#include "Game.h"
// レベルアップUIは製品ビルドでも操作できるようにする。
#include "imgui_impl_win32.h"
#include "imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);
const auto ClassName = TEXT("2022 framework ひな型"); //ウィンドウクラス名
const auto WindowName = TEXT("2023 framework ひな型"); //ウィンドウ名

HINSTANCE Application::m_hInst; // インスタンスハンドル
HWND Application::m_hWnd; // ウィンドウハンドル
uint32_t Application::m_width; // ウィンドウの横幅
uint32_t Application::m_height; // ウィンドウの縦幅

//-----------------------------------------------------------------------------
// コンストラクタ
//-----------------------------------------------------------------------------
Application::Application(uint32_t width, uint32_t height) {
    m_height = height;
    m_width = width;

    timeBeginPeriod(1); //タイマー精度を1ミリ秒に設定
}

//-----------------------------------------------------------------------------
// デストラクタ
//-----------------------------------------------------------------------------
Application::~Application() {
    timeEndPeriod(1); // タイマー精度を元に戻す
}

//-----------------------------------------------------------------------------
// 実行
//-----------------------------------------------------------------------------
void Application::Run() {
    //初期化
    bool okfg = InitApp();
    if (okfg) {
        MainLoop();
    }

    UninitApp(); // 終了処理
}

//-----------------------------------------------------------------------------
// 初期化処理
//-----------------------------------------------------------------------------
bool Application::InitApp() {
    SetProcessDPIAware();

    // インスタンスハンドルを取得
    auto hInst = GetModuleHandle(nullptr);
    if (hInst == nullptr) {
        return false;
    }

    // ウィンドウの設定
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hInst, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = ClassName;
    wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

    // ウィンドウの登録
    if (!RegisterClassEx(&wc)) {
        return false;
    }

    // インスタンスハンドル設定
    m_hInst = hInst;

    // ウィンドウのサイズを設定
    RECT rc = {};
    rc.right = static_cast<LONG>(m_width);
    rc.bottom = static_cast<LONG>(m_height);

    // ウィンドウサイズを調整
    auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&rc, style, FALSE);

    // ウィンドウを生成
    m_hWnd = CreateWindowEx(0,
                            //        WS_EX_TOPMOST,
                            ClassName, WindowName, style, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
                            rc.bottom - rc.top, nullptr, nullptr, m_hInst, nullptr);

    if (m_hWnd == nullptr) {
        return false;
    }

    SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP | WS_MINIMIZEBOX);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(m_hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // ウィンドウを表示
    ShowWindow(m_hWnd, SW_SHOWNORMAL);

    // ウィンドウを更新
    UpdateWindow(m_hWnd);

    // ウィンドウにフォーカスを設定
    SetFocus(m_hWnd);

    // 正常終了
    return true;
}

//-----------------------------------------------------------------------------
// 終了処理
//-----------------------------------------------------------------------------
void Application::UninitApp() {
    // ウィンドウの登録を解除
    if (m_hInst != nullptr) {
        UnregisterClass(ClassName, m_hInst);
    }

    m_hInst = nullptr;
    m_hWnd = nullptr;
}

//-----------------------------------------------------------------------------
// メインループ
//-----------------------------------------------------------------------------
void Application::MainLoop() {
    MSG msg{};
    Game::Init();

    LARGE_INTEGER frequency{};
    LARGE_INTEGER previous{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&previous);

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        LARGE_INTEGER current{};
        QueryPerformanceCounter(&current);

        // 最小化中も基準時刻を更新し、復帰時に停止時間をまとめて進めない。
        if (IsIconic(m_hWnd)) {
            previous = current;
            Sleep(10);
            continue;
        }

        // 前回の更新開始から今回の更新開始までを、高精度カウンターで秒に換算する。
        // 待機時間だけでなく、前フレームの更新・描画にかかった時間も含む。
        const double elapsedSeconds =
            static_cast<double>(current.QuadPart - previous.QuadPart) / frequency.QuadPart;

        // 描画の上限は従来どおり60fps。ゲームの時間には固定値ではなく実測値を渡す。
        if (elapsedSeconds < 1.0 / 60.0) {
            continue;
        }
        previous = current;

        Game::Update(elapsedSeconds);
        Game::Draw();
    }

    Game::Uninit();
}

//-----------------------------------------------------------------------------
// ウィンドウプロシージャ
//-----------------------------------------------------------------------------
LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool isFullscreen = true;
    static bool isMessageBoxShowed = false;
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
        return true;
    }

    switch (uMsg) {
        case WM_DESTROY: // ウィンドウ破棄のメッセージ
            PostQuitMessage(0); // 「WM_QUIT」メッセージを送る　→　アプリ終了
            break;

        case WM_CLOSE: // 「x」ボタンが押されたら
        {
            DestroyWindow(hWnd); // 「WM_DESTROY」メッセージを送る
            int res = MessageBoxA(NULL, "終了しますか？", "確認", MB_OKCANCEL);
            if (res == IDOK) {
            }
        } break;

        case WM_KEYDOWN: //キー入力があったメッセージ
            if (LOWORD(wParam) == VK_ESCAPE) { //入力されたキーがESCAPEなら
                PostMessage(hWnd, WM_CLOSE, wParam, lParam); //「WM_CLOSE」を送る
            }

            else if (LOWORD(wParam) == VK_F11) {
                isFullscreen = !isFullscreen;
                if (isFullscreen) {
                    //フルスクリーンに切り替え
                    //g_pSwapChain->SetFullscreenState(TRUE, NULL);
                    //ShowWindow(hWnd, SW_MAXIMIZE);

                    // 疑似フルスクリーンモードに変更
                    SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_MINIMIZEBOX); // ウィンドウ枠を削除
                    // ディスプレイ解像度を取得
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    SetWindowPos(hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight,
                                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                } else {
                    //ウィンドウモードに戻す
                    //g_pSwapChain->SetFullscreenState(FALSE, NULL);
                    //ShowWindow(hWnd, SW_RESTORE);

                    // 通常ウィンドウに戻す
                    SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW); // ウィンドウ枠を戻す
                    SetWindowPos(hWnd, HWND_TOP, 100, 100, m_width, m_height,
                                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                }
            }
            break;

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                // フルスクリーン表示かつメッセージボックス非表示なら
                if (isFullscreen && !isMessageBoxShowed) {
                    // ウインドウを最小化する（タスク切替時に背後に残る問題対策）
                    ShowWindow(hWnd, SW_MINIMIZE);
                }
            }
            // 標準挙動を実行
            return DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_SIZE: //ウィンドウサイズに変更があったメッセージ

            if (wParam != SIZE_MINIMIZED) {
                int width = LOWORD(lParam); //横幅
                int height = HIWORD(lParam); //縦幅
                Renderer::ResizeWindow(width, height);
            }
            break;

        case WM_SETCURSOR: //カーソルの形を変更するメッセージ
            if (LOWORD(lParam) == HTCLIENT) { // ウィンドウのクライアント領域内であれば

#ifdef _DEBUG

                if (ImGui::GetIO().WantCaptureMouse) {
                    return DefWindowProc(hWnd, uMsg, wParam, lParam);
                }
#endif // _DEBUG

                SetCursor(nullptr); // カーソルを非表示にする
                return TRUE; // ウィンドウプロシージャで処理したことをOSに伝える
            }
            break;

        default:
            // 受け取ったメッセージに対してデフォルトの処理を実行
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return 0;
}
