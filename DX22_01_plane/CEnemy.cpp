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
	m_body->SetScale(1.0f, 1.0f, 1.0f); // 敵の体
}

void CEnemy::Update() {
	std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();

	if (!grounds.empty()) {//groundsの中が空っぽかそうか１つ以上作られたか
		Ground* plane = grounds[0];
		float planeY = plane->GetPosition().y+10.0f ;//床座標の取得 + 微調整

		if (m_Position.y <= planeY) {
			m_Position.y = planeY;
			Onland();
		}
		else {
			//空中状態
			IsGrounded = false;
		}
	}

    
    

	m_body->SetPositin(m_Position.x, m_Position.y, m_Position.z); // 仮置き

    if (hp <= 0) {//一番下！
        Game::GetInstance()->DeleteObject(this); // 体力が0以下になったらオブジェクトを削除
        return;
    }
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

void CEnemy::OnHit(int& damage) {
    hp -= damage; // 体力を減らす
    std::cout << "Enemy hit! Remaining HP: " << hp << std::endl; // デバッグ用に体力を表示
}
