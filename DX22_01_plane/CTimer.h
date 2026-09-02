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
    static constexpr int DIGIT_COUNT = 3;  // 表示する桁数（例：000〜999秒）
    static constexpr int ATLAS_SPLIT_X = 10; // number.pngの横分割数（0〜9で10コマ）
    std::vector<Texture2D*> m_DigitTextures; // 桁ごとのTexture2D

    float m_DigitWidth = 52.0f; // 1桁の表示幅
    float m_DigitHeight = 72.0f; // 1桁の表示高さ
    float m_DigitSpacing = 56.0f; // 桁と桁の間隔

public:
    CTimer();
    ~CTimer();
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // ObjectのSetPosition(非virtual)を上書きし、位置が変わったら子も追従させる
    Object* SetPosition(float x, float y, float z);

private:
    void RelayoutDigits();     // 位置を並べ直す
};

