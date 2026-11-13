/// @brief 回転するジオラマと開始ボタンを表示するタイトル画面。
#pragma once
#include "Scene.h"

class TitleScene : public Scene {
public:
    TitleScene();
    ~TitleScene() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void DrawUI() override;

private:
    struct Diorama;
    std::unique_ptr<Diorama> m_diorama;
};
