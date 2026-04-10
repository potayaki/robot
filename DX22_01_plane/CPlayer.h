#pragma once
#include "Object.h"
#include"TestCube.h"
#include"GolfBall.h"
class CPlayer : public Object
{
private:
	static const int MaxBullet = 5;//最大弾数
	TestCube* m_body;//body
	
	TestCube* m_bullets[MaxBullet];//弾

	DirectX::SimpleMath::Vector3 m_velocity = { 0,0,0 };
	bool IsGrounded = false;//着いていた

	
public:
	CPlayer();
	~CPlayer();

	void Init()override;
	void Update()override;
	void Draw(Camera* cam) override;
	void Uninit()override;

	void Move();

	DirectX::SimpleMath::Vector3 GetVelocity() { return m_velocity; }
	void SetVelocity(float y) { m_velocity.y = y; }
	//void SetMaterial(DirectX::SimpleMath::Color col);
	//着地中の処理.
	void Onland();

	//当たり判定用のBOXを取得
	DirectX::SimpleMath::Vector3 GetPosition()const {return  m_Position; }
	DirectX::SimpleMath::Vector3 GetScale() const{ return m_Scale; }
	
	// 当たり判定用（車体の位置を返す）
	DirectX::SimpleMath::Vector3 GetPosition() { return m_Position; }


};

