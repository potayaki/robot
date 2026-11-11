/// @file CPresentBox.h
/// @brief 敵が落とすプレゼントの表示、取得、ランダム報酬。

#pragma once
#include "Object.h"
#include "TestModel.h"
#include <vector>

class CPresentBox : public Object {
private:
    /// プレゼントが回転する速さ（ラジアン/秒）。
    float m_rotationSpeed = 3.0f;

    /// 回転角に対する上下動の周期倍率。
    float m_bobFrequency = 2.0f;

    /// 上下動に合わせて拡大縮小する量。
    float m_pulseScale = 0.01f;

    /// 判定位置に対してモデルを上下にずらす量。
    float m_modelHeightOffset = -4.0f;

    /// このオブジェクトの見た目を描画するモデル。
    TestModel* m_model = nullptr;

    /// 当たり判定用の半径
    float m_radius = 5.0f;
    /// 回転角度
    float m_angle = 0.0f;
    /// モデルのスケール
    float m_scale = 0.055f;
    /// 生成されたプレゼントボックスのリスト
    static TestModel* m_sharedModel;
    /// 最大生成数
    static int m_boxCount;
    /// 最大生成数
    int m_maxBoxCount = 10;

public:
    CPresentBox();
    ~CPresentBox();
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    static void UnloadResource();
    static void LoadResource();

    Collision::Sphere GetCollisionSphere() override {
        return Collision::Sphere{m_position, m_radius};
    }
};
