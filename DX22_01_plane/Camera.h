#pragma once

#include	<SimpleMath.h>

//-----------------------------------------------------------------------------
//Cameraクラス
//-----------------------------------------------------------------------------
class Camera {
private:
	DirectX::SimpleMath::Vector3	m_Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Rotation = DirectX::SimpleMath::Vector3(-10.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3	m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

	DirectX::SimpleMath::Vector3	m_Target{};
	DirectX::SimpleMath::Matrix		m_ViewMatrix{};

	float m_CameraDirection = 0;

	float speed = 1.0f;

	DirectX::SimpleMath::Vector3 offset = DirectX::SimpleMath::Vector3(0.0f, 30.0f, -100.0f);

public:
	void Init();
	void Update();
	void SetCamera(int mode = 0);
	void Uninit();
	void SetPosition(DirectX::SimpleMath::Vector3 camera_pos) { m_Position = camera_pos; }
	void SetTarget(DirectX::SimpleMath::Vector3 target);

    bool GetMouseRay(DirectX::SimpleMath::Vector3& rayOrigin, DirectX::SimpleMath::Vector3& rayDirection)const;

};
