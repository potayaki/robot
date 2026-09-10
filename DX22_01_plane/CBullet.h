#pragma once
/*
Playerで使われる弾のクラス
*/
#include"TestModel.h"
#include "Object.h"
#include"PoolManager.h"
using namespace DirectX::SimpleMath;
class CBullet :	public Object
{
private:
    TestModel* m_model;//弾のモデル
	Vector3 m_velocity;//弾の方向
    float Speed = 25.0f;
	int m_life;//寿命
    int damage = 1; // 弾のダメージ量
    Vector3 a{0.0f, 0.0f, 0.0f}; // 弾の初期方向（例: 前方）

    bool isActive = true; // 弾がアクティブかどうかのフラグ

    float m_colRadius = 2.0f;//当たり判定の半径

public:
	CBullet();
		~CBullet();

      
		
		void Init()override;//初期化処理
		void Update()override;//更新処理
		void Draw(Camera* cam) override;//描画処理
		void Uninit()override;//終了処理(モデルの破棄)

        bool GetActive() { return isActive; }

        void SetActive(bool active) { isActive = active; }  

		void SetVelocity(Vector3 velocity) { m_velocity = velocity; }
		Vector3 GetVelocity() { return m_velocity; };
	
	// 最初に発射方向と初期位置をする関数
	void Shoot(DirectX::SimpleMath::Vector3 player, DirectX::SimpleMath::Vector3 dir);

    //当たり判定用の球を返す関数
    Collision::Sphere GetCollisionSphere()override {
        return { m_Position,m_colRadius };
    }

    //当たり判定用の球を返す関数
    void SetColRadius(float radius) { m_colRadius = radius; }

};
using BulletManager = PoolManager<CBullet, 30>; // 30個の弾を管理するプールマネージャ

