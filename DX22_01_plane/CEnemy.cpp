#include "CEnemy.h"
#include"Ground.h"
#include"Game.h"
CEnemy::CEnemy() {
	m_body = nullptr;

}

CEnemy::~CEnemy() {
	Uninit();
}

void CEnemy::Init() {
	m_body = new TestCube;
	m_body->Init();
	m_body->SetScale(1.0f, 1.0f, 1.0f); // “G‚Ì‘Ì
}

void CEnemy::Update() {
	std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();

	if (!grounds.empty()) {//grounds‚Ì’†‚ª‹ó‚Á‚Û‚©‚»‚¤‚©‚P‚ÂˆÈãì‚ç‚ê‚½‚©
		Ground* plane = grounds[0];
		float planeY = plane->GetPosition().y+10.0f ;//°À•W‚Ìæ“¾ + ”÷’²®

		if (m_Position.y <= planeY) {
			m_Position.y = planeY;
			Onland();
		}
		else {
			//‹ó’†ó‘Ô
			IsGrounded = false;
		}
	}

	m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z); // ‰¼’u‚«

}

void CEnemy::Draw(Camera* cam) {
		if (m_body) {
		m_body->Draw(cam);
	}

}

void CEnemy::Uninit() {
	if (m_body) {
		m_body->Uninit();
		delete m_body;
		m_body = nullptr;
	}
}

void CEnemy::Onland() {
		IsGrounded = true;
		
}
