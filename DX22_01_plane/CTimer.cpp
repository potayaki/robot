#pragma once
#include "Object.h"
#include "Texture2D.h"
#include <vector>
#include"CTimer.h"
#include"Game.h"


CTimer::CTimer() {}
CTimer::~CTimer() { Uninit(); }

void CTimer::Init() {
    // 整数部の桁を作成
    m_IntDigits.resize(Int_Digit_Count);
    for (int i = 0; i < Int_Digit_Count; i++) {
        Texture2D* digit = Game::GetInstance()->AddUI<Texture2D>();
        digit->SetTexture("assets/texture/number.png");
        digit->SetScale(m_DigitWidth, m_DigitHeight, 1.0f);
        m_IntDigits[i] = digit;
    }

    // 小数点を作成（パスは実際に用意した画像に合わせて変更してください）
    m_DecimalPoint = Game::GetInstance()->AddUI<Texture2D>();
    m_DecimalPoint->SetTexture("assets/texture/miniwhite.png");
    m_DecimalPoint->SetScale(m_PointWidth, m_PointWidth, 1.0f);

    // 小数部の桁を作成
    m_DecDigits.resize(Dec_Digit_Count);
    m_IntDigits.resize(Int_Digit_Count);//
    for (int i = 0; i < Dec_Digit_Count; i++) {
        Texture2D* digit = Game::GetInstance()->AddUI<Texture2D>();
        digit->SetTexture("assets/texture/number.png");
        digit->SetScale(m_DigitWidth, m_DigitHeight, 1.0f);
        m_DecDigits[i] = digit;
        
    }

    RelayoutDigits();     // 位置を並べる
    UpdateDigitDisplay(); // 最初は "000.0" を表示
}

void CTimer::Update() {
    m_FrameCount++;
    if (m_FrameCount >= 60) // 60FPS想定で1秒（CPlayerのクールダウンと同じ考え方）
    {
        m_FrameCount = 0;
        m_Seconds++;
    }

    UpdateDigitDisplay(); // 小数部は毎フレーム動くので、ここは毎フレーム呼ぶ
}

void CTimer::Draw(Camera* cam) {

    // Game::Draw()が自動的に描画してくれる。
}

void CTimer::Uninit() {
    for (Texture2D* d : m_IntDigits) { if (d) d->Destroy(); }
    for (Texture2D* d : m_DecDigits) { if (d) d->Destroy(); }
    if (m_DecimalPoint) m_DecimalPoint->Destroy();
}

Object* CTimer::SetPosition(float x, float y, float z) {
    m_Position = DirectX::SimpleMath::Vector3(x, y, z);
    RelayoutDigits();
    return this;
}

void CTimer::ResetTimer() {
    m_Seconds = 0;
    m_FrameCount = 0;
    UpdateDigitDisplay();
}

// 整数部→小数点→小数部の順に、左から右へ並べる
void CTimer::RelayoutDigits() {
    float x = m_Position.x;

    for (int i = 0; i < Int_Digit_Count; i++) {
        m_IntDigits[i]->SetPosition(x, m_Position.y, m_Position.z);
        x += m_DigitSpacing;
    }

    m_DecimalPoint->SetPosition(x - 10.5f, m_Position.y - 18.0f, m_Position.z);
    x += m_PointSpacing;

    for (int i = 0; i < Dec_Digit_Count; i++) {
        m_DecDigits[i]->SetPosition(x, m_Position.y, m_Position.z);
        x += m_DigitSpacing;
    }
}

void CTimer::UpdateDigitDisplay() {
    // ① 整数部：m_Secondsを1桁ずつ取り出す
    int intValue = m_Seconds;
    for (int i = Int_Digit_Count - 1; i >= 0; i--) {
        SetDigitUV(m_IntDigits[i], intValue % 10);
        intValue /= 10;
    }

    // ② 小数部：60フレーム中の何フレーム目かを0〜9に変換する
    //    (frame=0→0, frame=6→1, ... frame=59→9)
    int decValue = (m_FrameCount * 10) / 60;
    for (int i = Dec_Digit_Count - 1; i >= 0; i--) {
        SetDigitUV(m_DecDigits[i], decValue % 10);
        decValue /= 10;
    }
}

// number.pngの(digit)番目のコマを表示する共通処理
void CTimer::SetDigitUV(Texture2D* tex, int digit) {
    // Texture2D::Draw内で"NumU-1"として使われるので1始まりで渡す
    tex->SetUV(
        (float)(digit + 1), // nu
        1.0f,                 // nv
        (float)SplitX, // sx
        1.0f);                // sy
}
