/// @brief 回転するジオラマと、ゲーム開始・設定・ルール・終了のタイトルメニュー。
#pragma once
#include "Scene.h"
#include <optional>

enum class TitleEnemyKind;

class TitleScene : public Scene {
public:
    TitleScene();
    ~TitleScene() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void DrawUI() override;

private:
    void DrawMenuPanel();
    /// 選んだ敵の解説と、前後のフィギュアへ移る操作を表示する。
    void DrawEnemyInspector();
    void UpdateTurntableDrag();
    struct Diorama;
    std::unique_ptr<Diorama> m_diorama;
    /// フィギュアの解説を開いている間だけ敵の種類を保持する。
    std::optional<TitleEnemyKind> m_inspectedEnemy;
};
