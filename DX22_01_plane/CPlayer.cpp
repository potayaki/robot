#include "CPlayer.h"
#include"Ground.h"
#include"Game.h"
#include"input.h"
using namespace DirectX::SimpleMath;

namespace {
	const float Gravity = 0.008f;
	const float speed= 0.1f;
	const float JumpPower = 0.25f;
}

CPlayer::CPlayer() {
	m_body = nullptr;
	IsGrounded = false;
	m_velocity = { 0,0,0 };

}

CPlayer::~CPlayer() {
	Uninit();
}

void CPlayer::Init() {
	m_body = new TestCube;
	m_body->Init();
	m_body->SetScale(1.0f, 1.0f, 1.0f); // バイク本体

	for (size_t i = 0; i < MaxBullet; i++) {
		m_bullets[i] = new TestCube;
		m_bullets[i]->Init();
		m_bullets[i]->SetScale(0.2f, 0.2f, 0.2f); // 弾のサイズ
		m_bullets[i]->SetTexture("assets/texture/arrow.png"); // 弾のテクスチャ
	}

}


void CPlayer::Update() {
	
  	m_velocity.y -= Gravity;
	m_Position += m_velocity;
	std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();

	if (!grounds.empty()) {//groundsの中が空っぽかそうか１つ以上作られたか
		Ground *plane = grounds[0];
		float planeY = plane->GetPosition().y + 10.0f;//床座標の取得 + 微調整

		if (m_Position.y<= planeY) {
			m_Position.y = planeY;
			Onland();
		}
		else {
			//空中状態
			IsGrounded = false;
		}

	}

	if (Input::GetKeyTrigger(VK_SPACE) && IsGrounded) {
		m_velocity.y = JumpPower;
		
		
	}

	Move();

	
	m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z); // 仮置き
}

void CPlayer::Draw(Camera* cam) {
	if (m_body) {
		m_body->Draw(cam);
	}

}

void CPlayer::Onland() {
	IsGrounded = true;
	m_velocity.y = 0;
}
void CPlayer::Uninit() {
	if (m_body) {
		m_body->Uninit();
		delete m_body;
		m_body = nullptr;
	}
}

void CPlayer::Move() {
	if (Input::GetKeyPress(VK_A)) {
		m_Position.x -= speed;
	}
	if (Input::GetKeyPress(VK_D)) {
		m_Position.x += speed;
	}
	if (Input::GetKeyPress(VK_W)) {
		m_Position.z += speed;
	}
	if (Input::GetKeyPress(VK_S)) {
		m_Position.z -= speed;
	}
}

