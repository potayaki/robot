#pragma once

#include	<SimpleMath.h>

/*
ゲームのカメラを管理するクラス
*/
class Camera {
private:
    //カメラの基本情報
	DirectX::SimpleMath::Vector3	m_Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Rotation = DirectX::SimpleMath::Vector3(-10.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

    //注視点(カメラが見る方向)
	DirectX::SimpleMath::Vector3	m_Target{};

    //カメラのビュー行列
	DirectX::SimpleMath::Matrix		m_ViewMatrix{};

    //カメラの水平回転角
	float m_CameraDirection = 0;

    //ウィンドウ幅
    int m_Width;

    //プレイヤー追従時のオフセット
	DirectX::SimpleMath::Vector3 offset = DirectX::SimpleMath::Vector3(0.0f, 30.0f, -150.0f);

    //カメラの揺れ表現
    float m_ShakeTime = 0.0f; // 揺れの残り時間（秒）。
    float m_ShakePower = 0.0f;

#ifdef _DEBUG
    // 一時停止中だけ使う視点。通常の追従位置を保存し、再開時にそのまま戻す。
    bool m_debugFreeCamera = false;
    DirectX::SimpleMath::Vector3 m_debugSavedPosition{};
    DirectX::SimpleMath::Vector3 m_debugSavedTarget{};
    float m_debugYawDegrees = 0.0f;
    float m_debugPitchDegrees = 0.0f;
    float m_debugMoveSpeed = 100.0f; // ワールド座標での移動量 / 秒。

    void ResetDebugFreeCamera();
    void ApplyDebugFreeLook();
#endif

public:
	void Init();
	void Update();
	void SetCamera(int mode = 0);
	void Uninit();
#ifdef _DEBUG
    void SetDebugFreeCamera(bool enabled);
    void UpdateDebugFreeCamera();
    void DrawDebugFreeCameraUI();
#endif
	void SetPosition(DirectX::SimpleMath::Vector3 camera_pos) { m_Position = camera_pos; }
	void SetTarget(DirectX::SimpleMath::Vector3 target);

    //マウスレイ取得
    bool GetMouseRay(DirectX::SimpleMath::Vector3& rayOrigin, DirectX::SimpleMath::Vector3& rayDirection)const;

    DirectX::SimpleMath::Matrix GetViewMatrix() const { return m_ViewMatrix; }

    //Debug用のオフセット
    DirectX::SimpleMath::Vector3 Getoffset() const { return offset; }
    void Setoffset(DirectX::SimpleMath::Vector3 off) { offset = off; }




    float GetCameraDirection()const {
        return m_CameraDirection;
    }

    //カメラシェイク開始。timeは持続秒数。
    void SetShake(float time, float power) {
        m_ShakeTime = time;
        m_ShakePower = power;
    }

};
