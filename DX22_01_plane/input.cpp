/// @file input.cpp
/// @brief キーボード、マウス、ゲームパッドの入力状態と振動。

#include "input.h"
#include "Application.h"
#include "GameTime.h"

Input* Input::m_instance = {};

POINT Input::m_mousePosition = {};

void Input::Create() {
    if (m_instance) {
        return;
    }
    m_instance = new Input;

    m_instance->m_vibrationTime = 0;
}

void Input::Update() {
    POINT mouseposition;
    GetCursorPos(&mouseposition);
    ScreenToClient(Application::GetWindow(), &mouseposition);

    m_mousePosition = mouseposition;
    //1フレーム前の入力を記録しておく
    for (int i = 0; i < 256; i++) {
        m_instance->m_previousKeyState[i] = m_instance->m_keyState[i];
    }
    m_instance->m_previousControllerState = m_instance->m_controllerState;

    //キー入力を更新
    BOOL hr = GetKeyboardState(m_instance->m_keyState);

    //コントローラー入力を更新(XInput)
    XInputGetState(0, &(m_instance->m_controllerState));

    //振動継続時間をカウント
    if (m_instance->m_vibrationTime > 0) {
        GameTime::CountDown(m_instance->m_vibrationTime, GameTime::UnscaledDeltaSeconds());
        if (m_instance->m_vibrationTime == 0) { //振動継続時間が経った時に振動を止める
            XINPUT_VIBRATION vibration;
            ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
            vibration.wLeftMotorSpeed = 0;
            vibration.wRightMotorSpeed = 0;
            XInputSetState(0, &vibration);
        }
    }
}

void Input::ConsumeTriggers() {
    // 押し続けている状態は維持し、「押した／離した瞬間」だけを消費する。
    for (int i = 0; i < 256; ++i) {
        m_instance->m_previousKeyState[i] = m_instance->m_keyState[i];
    }
    m_instance->m_previousControllerState = m_instance->m_controllerState;
}

void Input::Release() {
    //振動を終了させる
    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
    vibration.wLeftMotorSpeed = 0;
    vibration.wRightMotorSpeed = 0;
    XInputSetState(0, &vibration);

    //解放
    if (m_instance) {
        delete m_instance;
        m_instance = NULL;
    }
}

//キー入力
bool Input::GetKeyPress(int key) //プレス
{
    return m_instance->m_keyState[key] & 0x80;
}

bool Input::GetKeyTrigger(int key) //トリガー
{
    return (m_instance->m_keyState[key] & 0x80) && !(m_instance->m_previousKeyState[key] & 0x80);
}

bool Input::GetKeyRelease(int key) //リリース
{
    return !(m_instance->m_keyState[key] & 0x80) && (m_instance->m_previousKeyState[key] & 0x80);
}

//左アナログスティック
DirectX::XMFLOAT2 Input::GetLeftAnalogStick(void) {
    SHORT x = m_instance->m_controllerState.Gamepad.sThumbLX; // -32768～32767
    SHORT y = m_instance->m_controllerState.Gamepad.sThumbLY; // -32768～32767

    DirectX::XMFLOAT2 res = {};
    res.x = x / 32767.0f; //-1～1
    res.y = y / 32767.0f; //-1～1
    return res;
}

//右アナログスティック
DirectX::XMFLOAT2 Input::GetRightAnalogStick(void) {
    SHORT x = m_instance->m_controllerState.Gamepad.sThumbRX; // -32768～32767
    SHORT y = m_instance->m_controllerState.Gamepad.sThumbRY; // -32768～32767

    DirectX::XMFLOAT2 res = {};
    res.x = x / 32767.0f; //-1～1
    res.y = y / 32767.0f; //-1～1
    return res;
}

//左トリガー
float Input::GetLeftTrigger(void) {
    BYTE t = m_instance->m_controllerState.Gamepad.bLeftTrigger; // 0～255
    return t / 255.0f;
}

//右トリガー
float Input::GetRightTrigger(void) {
    BYTE t = m_instance->m_controllerState.Gamepad.bRightTrigger; // 0～255
    return t / 255.0f;
}

//ボタン入力
bool Input::GetButtonPress(WORD btn) //プレス
{
    return (m_instance->m_controllerState.Gamepad.wButtons & btn) != 0;
}

bool Input::GetButtonTrigger(WORD btn) //トリガー
{
    return (m_instance->m_controllerState.Gamepad.wButtons & btn) != 0 &&
           (m_instance->m_previousControllerState.Gamepad.wButtons & btn) == 0;
}

bool Input::GetButtonRelease(WORD btn) //リリース
{
    return (m_instance->m_controllerState.Gamepad.wButtons & btn) == 0 &&
           (m_instance->m_previousControllerState.Gamepad.wButtons & btn) != 0;
}

//振動
void Input::SetVibration(float seconds, float powor) {
    // XINPUT_VIBRATION構造体のインスタンスを作成
    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

    // モーターの強度を設定（0～65535）
    vibration.wLeftMotorSpeed = (WORD)(powor * 65535.0f);
    vibration.wRightMotorSpeed = (WORD)(powor * 65535.0f);
    XInputSetState(0, &vibration);

    //振動継続時間を代入
    m_instance->m_vibrationTime = seconds; // 振動の持続時間は秒で指定する。
}

DirectX::XMFLOAT2 Input::GetMousePosition() {
    RECT clientRect{};
    GetClientRect(Application::GetWindow(), &clientRect);

    const float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
    const float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

    // Texture2D用の座標系へ変換
    // 原点は画面中央、Y軸は上方向がプラス
    const float x =
        m_mousePosition.x * Application::GetWidth() / clientWidth - Application::GetWidth() * 0.5f;

    const float y =
        Application::GetHeight() * 0.5f - m_mousePosition.y * Application::GetHeight() / clientHeight;

    return DirectX::XMFLOAT2(x, y);
}
