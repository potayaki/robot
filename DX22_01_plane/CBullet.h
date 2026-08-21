#pragma once
#include"TestModel.h"
#include "Object.h"
#include"PoolManager.h"
using namespace DirectX::SimpleMath;
class CBullet :	public Object
{
private:
		TestModel* m_model;
	Vector3 m_velocity;//弾の方向
    float Speed = 25.0f;
	int m_life;//寿命
    int damage = 1; // 弾のダメージ量
    Vector3 a{0.0f, 0.0f, 0.0f}; // 弾の初期方向（例: 前方）

    bool isActive = true; // 弾がアクティブかどうかのフラグ

public:
	CBullet();
		~CBullet();

      
		
		void Init()override;
		void Update()override;
		void Draw(Camera* cam) override;
		void Uninit()override;

        bool GetActive() { return isActive; }

        void SetActive(bool active) { isActive = active; }  

		void SetVelocity(Vector3 velocity) { m_velocity = velocity; }
		Vector3 GetVelocity() { return m_velocity; };
	
	// 最初に発射方向と初期位置をする関数
	void Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir);
};
using BulletManager = PoolManager<CBullet, 200>; // 100個の弾を管理するプールマネージャ

