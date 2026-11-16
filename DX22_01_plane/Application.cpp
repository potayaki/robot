/// @file Application.cpp
/// @brief Windowsウィンドウの作成、メインループ、終了処理。

#include <chrono>
#include <algorithm>
#include <thread>
#include "Application.h"
#include "Game.h"
#include "GameSettings.h"
// レベルアップUIは製品ビルドでも操作できるようにする。
#include "imgui_impl_win32.h"
#include "imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);
const auto ClassName = TEXT("2022 framework ひな型"); //ウィンドウクラス名
const auto WindowName = TEXT("2023 framework ひな型"); //ウィンドウ名

namespace {
    // 終了確認を取り消した次のフレームで、ダイアログの待ち時間を計測から外す。
    bool resumeAfterCloseDialog = false;
} // namespace

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
    GameSettings::Load();
    //初期化
    bool okfg = InitApp();
    if (okfg) {
        MainLoop();
    }

    UninitApp(); // 終了処理
    GameSettings::Shutdown();
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

    ApplyDisplayMode(GameSettings::Fullscreen());

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
// 表示モードに合わせてウィンドウの枠、サイズ、位置を設定する。
//-----------------------------------------------------------------------------
void Application::ApplyDisplayMode(bool fullscreen) {
    if (!m_hWnd) {
        return;
    }
    MONITORINFO monitor{sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
        return;
    }
    DWORD style = fullscreen ? (WS_POPUP | WS_MINIMIZEBOX) : WS_OVERLAPPEDWINDOW;
    RECT bounds = monitor.rcMonitor;
    if (!fullscreen) {
        RECT desired{0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
        AdjustWindowRect(&desired, style, FALSE);
        const RECT& work = monitor.rcWork;
        const LONG width = (std::min)(desired.right - desired.left, work.right - work.left);
        const LONG height = (std::min)(desired.bottom - desired.top, work.bottom - work.top);
        bounds.left = work.left + (work.right - work.left - width) / 2;
        bounds.top = work.top + (work.bottom - work.top - height) / 2;
        bounds.right = bounds.left + width;
        bounds.bottom = bounds.top + height;
    }
    SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
    SetWindowPos(m_hWnd, nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
                 bounds.bottom - bounds.top, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
}

// 終了処理
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
        if (resumeAfterCloseDialog) {
            previous = current;
            resumeAfterCloseDialog = false;
            continue;
        }

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

        // 描画の上限を変えても、ゲームの時間には同じ実測値を渡す。
        if (elapsedSeconds < 1.0 / GameSettings::FrameLimit()) {
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
            if (!isMessageBoxShowed) {
                isMessageBoxShowed = true;
                const int choice = MessageBoxW(hWnd, L"ゲームを終了しますか？", L"ゲーム終了",
                                               MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                isMessageBoxShowed = false;
                if (choice == IDYES) {
                    DestroyWindow(hWnd);
                } else {
                    resumeAfterCloseDialog = true;
                }
            }
        } break;

        case WM_KEYDOWN: //キー入力があったメッセージ
            if (LOWORD(wParam) == VK_ESCAPE) {
                // 開いているメニューではEscを「戻る」に使う。キーリピートで終了へ進めない。
                auto* game = Game::GetInstance();
                const bool titlePanelOpen = ImGui::GetCurrentContext() && game &&
                                            dynamic_cast<TitleScene*>(game->GetScene()) &&
                                            ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
                if (!titlePanelOpen && (lParam & (1L << 30)) == 0) {
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                }
            }

            else if (LOWORD(wParam) == VK_F11 && !(lParam & (1LL << 30))) {
                GameSettings::SetFullscreen(!GameSettings::Fullscreen());
            }
            break;

        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                // フルスクリーン表示かつメッセージボックス非表示なら
                if (GameSettings::Fullscreen() && !isMessageBoxShowed) {
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
