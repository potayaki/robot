#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include "input.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Camera::Init()
{
	m_Position = Vector3(0.0f, 20.0f, -50.0f);
	m_Target = Vector3(0.0f, 0.0f, 0.0f);
	m_CameraDirection = 3.14f;
	
}


//=======================================
//更新処理
//=======================================
void Camera::Update()
{
	if (Input::GetKeyPress(VK_K)) {
		m_CameraDirection += 0.02f;
	}
	if (Input::GetKeyPress(VK_L)) {
		m_CameraDirection -= 0.02f;
	}

	if (Input::GetKeyPress(VK_LEFT)) {
		m_Target.x -= speed;
	}
	if (Input::GetKeyPress(VK_RIGHT)) {
		m_Target.x += speed;
	}
	if (Input::GetKeyPress(VK_UP)) {
		m_Target.z += speed;
	}
	if (Input::GetKeyPress(VK_DOWN)) {
		m_Target.z -= speed;
	}
	
	Vector3 pos = m_Target;
	pos.y += 20;
	pos.x += sin(m_CameraDirection) * 50;
	pos.z += cos(m_CameraDirection) * 50;
	m_Position = pos;
	


}

//=======================================
//描画処理
//=======================================
void Camera::SetCamera(int mode)
{
	if (mode == 0) {//３D

	// ビュー変換後列作成
	Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_Position, m_Target, up); //左手系

	Renderer::SetViewMatrix(&m_ViewMatrix);

	//プロジェクション行列の生成
	constexpr float fieldOfView = DirectX::XMConvertToRadians(45.0f);    // 視野角
	
	float aspectRatio = static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::GetHeight());	// アスペクト比	
	float nearPlane = 1.0f;       // ニアクリップ
	float farPlane = 1000.0f;      // ファークリップ

	//プロジェクション行列の生成
	Matrix projectionMatrix;
	projectionMatrix =  DirectX::XMMatrixPerspectiveFovLH( fieldOfView,  aspectRatio,  nearPlane,  farPlane);	//左手系

	Renderer::SetProjectionMatrix(&projectionMatrix);
	}
	else if (mode == 1) {//2D
		//ビュー変換行列作成
		Vector3 pos = Vector3(0.0f, 0.0f, -10.0f);
		Vector3 tgt = Vector3(0.0f, 0.0f, 1.0f);
		Vector3 up = Vector3(0.0f, 1.0f,0.0f);
		m_ViewMatrix = DirectX::XMMatrixLookAtLH(pos, tgt, up);
		Renderer::SetViewMatrix(&m_ViewMatrix);

		//プロジェクション行列の作成
		float nearPlane = 1.0f;
		float farPlane = 1000.0f;
		Matrix projectionMatrix = DirectX::XMMatrixOrthographicLH(static_cast<float>(Application::GetWidth()), static_cast<float>(Application::GetHeight()), nearPlane, farPlane);
		projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);
		Renderer::SetProjectionMatrix(&projectionMatrix);
	}
}


//=======================================
//終了処理
//=======================================
void Camera::Uninit()
{

}

void Camera::SetTarget(DirectX::SimpleMath::Vector3 target) {
	m_Target = target;
}

