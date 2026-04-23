#pragma once
#include"TestModel.h"
#include "Object.h"
using namespace DirectX::SimpleMath;
class CBullet :	public Object
{
private:
		TestModel* m_model;
	Vector3 m_velocity = { 0,0,0 };
	int m_life;//寿命
public:
	CBullet();
		~CBullet();

		
		void Init()override;
		void Update()override;
		void Draw(Camera* cam) override;
		void Uninit()override;

		void SetVelocity(Vector3 velocity) { m_velocity = velocity; }
		Vector3 GetVelocity() { return m_velocity; };
	
	// 発射方向と初期位置をセットする関数
	void Shoot(DirectX::SimpleMath::Vector3 pos, DirectX::SimpleMath::Vector3 dir);
};

