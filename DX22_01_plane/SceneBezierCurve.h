/// @file SceneBezierCurve.h
/// @brief 以前のベジェ曲線表示の試作用コード。現在のゲームでは使用しない。

#ifndef __SCENE_BEZIER_CURVE_H__
#define __SCENE_BEZIER_CURVE_H__

#include "SceneBase.hpp"
#include "DirectXTex/SimpleMath.h"
#include "ball.h"
#include <vector>

class SceneBezierCurve : public SceneBase {
public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 使用した資源を解放して終了する。
    void Uninit();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update(float tick);
    /// 現在の状態を画面へ描画する。
    void Draw();

private:
    /// 曲線表示の確認用に保持する経過秒数。
    float m_time = 0.0f;
    /// 曲線上を移動する再生速度の倍率。
    float m_speed = 1.0f;
    /// ベジェ曲線上の進行率（0～1）。
    float m_progress = 0.0f;

    /// 位置のキー、または形状を構成する座標の配列。
    std::vector<DirectX::XMFLOAT3> m_positions;
    /// ベジェ曲線の制御点を表示する球の一覧。
    std::vector<Ball> m_ball;

    DirectX::XMFLOAT3 GetBezierCurvePosition(float t);
    void Beem(float t, DirectX::XMFLOAT3 end, int loop);
};

#endif // __SCENE_BEZIER_CURVE_H__