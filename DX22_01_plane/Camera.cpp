#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include "input.h"
#include"CPlayer.h"
#include"Game.h"
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Camera::Init() {
	m_Position = Vector3(0.0f, 20.0f, -50.0f);
	m_Target = Vector3(0.0f, 0.0f, 0.0f);
	m_CameraDirection = 3.14f;

}


//=======================================
//更新処理
//=======================================
void Camera::Update() {
	//Debug用のカメラ操作

	
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
	


	Scene* CurrentScene = Game::GetInstance()->GetScene();
	if (dynamic_cast<Stage1Scene*>(CurrentScene) != nullptr) {//ステージ１シーンのとき

		std::vector<CPlayer*>player = Game::GetInstance()->GetObjects<CPlayer>();
		if (!player.empty()) {//プレイヤーがいるとき
			Vector3	playerpos = player[0]->GetPosition();
			m_Position = playerpos + offset;
			SetTarget(playerpos);
		}
		
	}


}

//=======================================
//描画処理
//=======================================
void Camera::SetCamera(int mode) {
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
		projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);	//左手系

		Renderer::SetProjectionMatrix(&projectionMatrix);
	}
	else if (mode == 1) {//2D
		//ビュー変換行列作成
		Vector3 pos = Vector3(0.0f, 0.0f, -10.0f);
		Vector3 tgt = Vector3(0.0f, 0.0f, 1.0f);
		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
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
void Camera::Uninit() {

}

void Camera::SetTarget(DirectX::SimpleMath::Vector3 target) {
	m_Target = target;
}

bool Camera::GetMouseRay(DirectX::SimpleMath::Vector3& rayOrigin, DirectX::SimpleMath::Vector3& rayDirection) const {
    const auto mouse = Input::GetMousePosition();

    const float halfWidth = Application::GetWidth() * 0.5f;
    const float halfHeight = Application::GetHeight() * 0.5f;

    //-1.0 ～ 1.0 に正規化
    const float ndcX = mouse.x / halfWidth;
        // Y軸はウィンドウ座標と3D空間で上下逆転するため、マイナスをつける
        const float ndcY = mouse.y / halfHeight;

    constexpr float fov = DirectX::XMConvertToRadians(45.0f);
    const float aspect =
        static_cast<float>(Application::GetWidth()) /
        static_cast<float>(Application::GetHeight());

    const float tanHalfFov = tanf(fov * 0.5f);

    Vector3 forward = m_Target - m_Position;
    if (forward.LengthSquared() < 0.0001f) {
        return false;
    }
    forward.Normalize();

    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = up.Cross(forward);
    right.Normalize();

    up = forward.Cross(right);
    up.Normalize();

    // 画面中央 (ndcX=0, ndcY=0) なら真正面 (forward) に飛ぶようになる
    rayDirection =
        forward +
        right * (ndcX * aspect * tanHalfFov) +
        up * (ndcY * tanHalfFov);

    rayDirection.Normalize();
    rayOrigin = m_Position;

    return true;
}

