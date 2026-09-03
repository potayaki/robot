#pragma once
#include "Object.h"
#include"Texture2D.h"
//#include"VertexBuffer.h"
//#include"Texture.h"
//#include"memory"
//#include"Material.h"
//#include"IndexBuffer.h"
class CTimer : public Object
{
private:

    static constexpr int Int_Digit_Count = 4;  // 整数部(秒)の桁数
    static constexpr int Dec_Digit_Count = 1;  // 少数部の桁数
    static constexpr int SplitX = 10; //横分割数（0〜9で10コマ）

    std::vector<Texture2D*> m_IntDigits;  // 整数部の桁
   std::vector<Texture2D*> m_DecDigits;  // 小数部の桁
    Texture2D* m_DecimalPoint = nullptr;  // 「.」用の画像

    int m_FrameCount = 0; // 現在の秒
    int m_Seconds = 0; // 経過秒数

    float m_DigitWidth = 52.0f; // 1桁の表示幅

    float m_DigitHeight = 72.0f; // 1桁の表示高さ
    float m_DigitSpacing = 35.0f; // 数字と数字の間隔
    float m_PointWidth = 10.0f; // 小数点の大きさ
    float m_PointSpacing = 24.0f; // 小数点の後にあける間隔


public:
    CTimer();
    ~CTimer();
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    Object* SetPosition(float x, float y, float z);
    void UpdateDigitDisplay(); // 現在の秒数を桁に反映させる
    void ResetTimer();
private:
    void RelayoutDigits();     // 位置を並べ直す
    void SetDigitUV(Texture2D* tex, int value); // 1桁の数字を表示するUVを設定
};

