#include "CEnemy.h"
#include"Ground.h"
#include"Game.h"
#include"CPlayer.h"
#include"CPresentBox.h"
//ランダム
#include<cstdlib>
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
    m_body->SetTexture("assets/texture/dice.png");
    m_body->SetMaterial(DirectX::SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f)); // 白色のマテリアル
}

void CEnemy::Update() {
    //地面との当たり判定-----------------------------------------------------------------------
	std::vector<Ground*>grounds = Game::GetInstance()->GetObjects<Ground>();
	if (!grounds.empty()) {//groundsの中が空っぽかそうか１つ以上作られたか
		Ground* plane = grounds[0];
		float planeY = plane->GetPosition().y+GetCollisionSphere().radius;//床座標の取得 +　自身の半径

		if (m_Position.y <= planeY) {
			m_Position.y = planeY;
			Onland();
		}
		else {
			//空中状態
			IsGrounded = false;
		}
	}

    //移動-----------------------------------------------------------
    std::vector<CPlayer*>player = Game::GetInstance()->GetObjects<CPlayer>();
    if (!player.empty() && player[0] != nullptr) {
        DirectX::SimpleMath::Vector3 PlayerPos = player[0]->GetPosition();

        DirectX::SimpleMath::Vector3 dir = PlayerPos - m_Position;
        dir.y = 0.0f;

        if (dir.LengthSquared() > 0.0001f) {
            dir.Normalize();

            m_Position += dir * m_Speed;
            m_Rotation.y = atan2f(dir.x, dir.z);
        }
    }
   
    
    

	m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z); // 仮置き

    if (hp <= 0) {//一番下！

        int dropChance = rand() % 100; // 0から99までのランダムな数を生成

        if (dropChance < m_Droppercent) {
            // プレゼントボックスを生成
            
            CPresentBox* presentBox = Game::GetInstance()->AddObject<CPresentBox>();
            presentBox->SetPosition(m_Position.x, m_Position.y + 10.0f, m_Position.z); // 少し上に出す
            

        }

        Destroy(); // 体力が0以下になったらオブジェクトを破棄
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
