#include "CTimer.h"
#include"Game.h"
using namespace DirectX::SimpleMath;

CTimer::CTimer() {
}

CTimer::~CTimer() {
}

void CTimer::Init() {
    m_DigitTextures.resize(DIGIT_COUNT);
    for (int i = 0; i < DIGIT_COUNT; i++) {
        Texture2D* digit = Game::GetInstance()->AddUI<Texture2D>();
        digit->SetTexture("assets/texture/number.png");
        digit->SetScale(m_DigitWidth, m_DigitHeight, 1.0f);
        m_DigitTextures[i] = digit;
    }
    RelayoutDigits();
    Update();
}

void CTimer::Update() {
    //Gameクラスの高精度な経過秒数を取得！
    int value = static_cast<int>(Game::GetInstance()->m_gameTime);

    for (int i = DIGIT_COUNT - 1; i >= 0; i--) {
        int digit = value % 10;
        value /= 10;

        // Claudeの指摘通り、1始まり(+1)で渡すことで綺麗に切り抜かれます
        m_DigitTextures[i]->SetUV(
            (float)(digit + 1),
            1.0f,
            (float)ATLAS_SPLIT_X,
            1.0f);
    }
}

void CTimer::Draw(Camera* cam) {
   
}

void CTimer::Uninit() {
    for (Texture2D* digit : m_DigitTextures) {
        if (digit) digit->Destroy();
    }
}

Object* CTimer::SetPosition(float x, float y, float z) {
    m_Position = DirectX::SimpleMath::Vector3(x, y, z);
    Update();
    return this;
    
}

void CTimer::RelayoutDigits() {
    for (int i = 0; i < DIGIT_COUNT; i++) {
        if (!m_DigitTextures[i]) continue;
        m_DigitTextures[i]->SetPosition(
            m_Position.x + m_DigitSpacing * (float)i,
            m_Position.y,
            m_Position.z);
    }
}
