/// @file Camera.h
/// @brief プレイヤー追従視点、カーソルのレイ、画面揺れ、自由カメラ。

#pragma once

#include <SimpleMath.h>

namespace AimCursor {
    class Region;
}

/*
ゲームのカメラを管理するクラス
*/
class Camera {
private:
    /// 画面端・デバッグキーで旋回する速さ（ラジアン/秒）。
    float m_rotationSpeed = 1.2f;

    /// マウス位置でカメラを回転させる画面端の幅（ピクセル）。
    int m_edgeWidth = 100;

    /// プレイヤー追従以外の画面で注視点から上げる高さ。
    float m_previewHeight = 20.0f;

    /// プレイヤー追従以外の画面で注視点を周回する半径。
    float m_previewRadius = 50.0f;

    /// 近距離の描画を切り取る距離。
    float m_nearPlane = 1.0f;

    /// 3D空間の遠距離の描画限界。
    float m_farPlane = 10000.0f;

    /// 2D描画で使う遠距離の描画限界。
    float m_uiFarPlane = 1000.0f;

    //カメラの基本情報
    /// カメラの位置（ワールド座標）。
    DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    /// 初期姿勢を保持するカメラの回転設定。
    DirectX::SimpleMath::Vector3 m_rotation = DirectX::SimpleMath::Vector3(-10.0f, 0.0f, 0.0f);
    /// モデルの拡大縮小倍率。
    DirectX::SimpleMath::Vector3 m_scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

    //注視点(カメラが見る方向)
    /// カメラが注視する位置（ワールド座標）。
    DirectX::SimpleMath::Vector3 m_target{};

    //カメラのビュー行列
    /// ワールド座標をカメラ座標へ変換するビュー行列。
    DirectX::SimpleMath::Matrix m_viewMatrix{};

    //カメラの水平回転角
    /// カメラの水平回転角（ラジアン）。
    float m_cameraDirection = 0;

    //ウィンドウ幅
    /// ウィンドウまたは画像の横幅（ピクセル）。
    int m_width;

    //プレイヤー追従時のオフセット
    /// プレイヤーを基準にした追従カメラの位置のずれ。
    DirectX::SimpleMath::Vector3 m_followOffset = DirectX::SimpleMath::Vector3(0.0f, 30.0f, -150.0f);

    //カメラの揺れ表現
    /// 揺れの残り時間（秒）。
    float m_shakeTime = 0.0f;
    /// 画面揺れでカメラ位置をずらす強さ。
    float m_shakePower = 0.0f;

#ifdef _DEBUG
    // 一時停止中だけ使う視点。通常の追従位置を保存し、再開時にそのまま戻す。
    /// 一時停止中の自由カメラ操作が有効か。
    bool m_debugFreeCamera = false;
    /// 自由カメラ開始前の通常カメラ位置。
    DirectX::SimpleMath::Vector3 m_debugSavedPosition{};
    /// 自由カメラ開始前の通常カメラ注視点。
    DirectX::SimpleMath::Vector3 m_debugSavedTarget{};
    /// 自由カメラの水平角度（度）。
    float m_debugYawDegrees = 0.0f;
    /// 自由カメラの上下角度（度）。
    float m_debugPitchDegrees = 0.0f;
    /// ワールド座標での移動量 / 秒。
    float m_debugMoveSpeed = 100.0f;

    void ResetDebugFreeCamera();
    void ApplyDebugFreeLook();
#endif

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update();
    void SetCamera(int mode = 0);
    /// 使用した資源を解放して終了する。
    void Uninit();
#ifdef _DEBUG
    void SetDebugFreeCamera(bool enabled);
    void UpdateDebugFreeCamera();
    void DrawDebugFreeCameraUI();
#endif
    void SetPosition(DirectX::SimpleMath::Vector3 camera_pos) {
        m_position = camera_pos;
    }

    void SetTarget(DirectX::SimpleMath::Vector3 target);

    //マウスレイ取得
    bool GetMouseRay(DirectX::SimpleMath::Vector3& rayOrigin,
                     DirectX::SimpleMath::Vector3& rayDirection) const;

    AimCursor::Region GetAimCursor() const;

    DirectX::SimpleMath::Matrix GetViewMatrix() const {
        return m_viewMatrix;
    }

    //Debug用のオフセット
    DirectX::SimpleMath::Vector3 Getoffset() const {
        return m_followOffset;
    }

    void Setoffset(DirectX::SimpleMath::Vector3 off) {
        m_followOffset = off;
    }

    float GetCameraDirection() const {
        return m_cameraDirection;
    }

    //カメラシェイク開始。timeは持続秒数。
    void SetShake(float time, float power) {
        m_shakeTime = time;
        m_shakePower = power;
    }
};
