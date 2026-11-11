/// @file CTimer.h
/// @brief 数字画像を組み合わせて表示するゲーム内経過時間。

#pragma once
#include "Object.h"
#include "Texture2D.h"

//#include"VertexBuffer.h"
//#include"Texture.h"
//#include"memory"
//#include"Material.h"
//#include"IndexBuffer.h"
class CTimer : public Object {
private:
    /// 数字の配置位置から小数点を横にずらす量（ピクセル）。
    float m_decimalPointOffsetX = -10.5f;

    /// 数字の配置位置から小数点を下げる量（ピクセル）。
    float m_decimalPointOffsetY = -18.0f;

    /// 整数部(秒)の桁数
    static constexpr int Int_Digit_Count = 4;
    /// 少数部の桁数
    static constexpr int Dec_Digit_Count = 1;
    /// 横分割数（0〜9で10コマ）
    static constexpr int SplitX = 10;

    /// 整数部の桁
    std::vector<Texture2D*> m_intDigits;
    /// 小数部の桁
    std::vector<Texture2D*> m_decDigits;
    /// 「.」用の画像
    Texture2D* m_decimalPoint = nullptr;

    /// リセット時点のゲーム内秒数。
    double m_startTime = 0.0;

    /// 1桁の表示幅
    float m_digitWidth = 52.0f;

    /// 1桁の表示高さ
    float m_digitHeight = 72.0f;
    /// 数字と数字の間隔
    float m_digitSpacing = 35.0f;
    /// 小数点の大きさ
    float m_pointWidth = 10.0f;
    /// 小数点の後にあける間隔
    float m_pointSpacing = 24.0f;

public:
    CTimer();
    ~CTimer();
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    Object* SetPosition(float x, float y, float z);
    void UpdateDigitDisplay(); // 現在の秒数を桁に反映させる
    void ResetTimer();

private:
    void RelayoutDigits(); // 位置を並べ直す
    void SetDigitUV(Texture2D* tex, int value); // 1桁の数字を表示するUVを設定
};
