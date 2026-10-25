#pragma once
#include "Object.h"
#include "Texture2D.h"
#include <vector>
#include"CTimer.h"
#include"Game.h"


CTimer::CTimer() {}
CTimer::~CTimer() { Uninit(); }

void CTimer::Init() {
    m_startTime = Game::GetInstance()->m_gameTime;
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
    UpdateDigitDisplay(); // 武器や敵の出現に使っているゲーム内秒数を表示する。
}

void CTimer::Draw(Camera* cam) {

    // 更新途中にレベルアップで停止しても、表示だけは現在のゲーム内時刻にそろえる。
    UpdateDigitDisplay();
    // 数字の画像自体は、後続のTexture2Dが描画する。
}

void CTimer::Uninit() {
    for (Texture2D* d : m_IntDigits) { if (d) d->Destroy(); }
    for (Texture2D* d : m_DecDigits) { if (d) d->Destroy(); }
    if (m_DecimalPoint) m_DecimalPoint->Destroy();
    // Gameの終了処理とデストラクタから二度呼ばれても、解放済みの数字を参照しない。
    m_IntDigits.clear();
    m_DecDigits.clear();
    m_DecimalPoint = nullptr;
}

Object* CTimer::SetPosition(float x, float y, float z) {
    m_Position = DirectX::SimpleMath::Vector3(x, y, z);
    RelayoutDigits();
    return this;
}

void CTimer::ResetTimer() {
    m_startTime = Game::GetInstance()->m_gameTime;
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
    // 実測時間を蓄積した共通の時計を使い、別のフレームカウンターは持たない。
    const double elapsedSeconds = Game::GetInstance()->m_gameTime - m_startTime;
    int intValue = static_cast<int>(elapsedSeconds);
    for (int i = Int_Digit_Count - 1; i >= 0; i--) {
        SetDigitUV(m_IntDigits[i], intValue % 10);
        intValue /= 10;
    }

    // 小数第1位は経過秒数から直接取り出す。
    int decValue = static_cast<int>(elapsedSeconds * 10.0) % 10;
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
