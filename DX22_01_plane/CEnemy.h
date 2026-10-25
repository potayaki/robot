#pragma once
/*
キャラクターの敵のクラス
*/
//#include "TestCube.h"	
#include"TestModel.h"
#include "EnemyTarget.h"
// 同じモデルを使っても、体格・移動・攻撃性能で異なる役割を持たせる。
enum class SlimeVariant { Normal, Runner, Brute };
class CEnemy :
    public EnemyTarget
{

    private:
        //敵の体
		TestModel* m_body;

        int hp = 3;//体力

        float m_Speed = 30.0f; // 移動速度（距離/秒）。プレイヤーは60。

		bool IsGrounded = false;//着いていた

        int m_Droppercent = 10;//プレゼントのドロップ率

        int damage = 2;//プレイヤーに与えるダメージ量

        float m_AttackTimer = 0.0f; // 攻撃のクールダウンタイマー
        SlimeVariant m_variant = SlimeVariant::Normal;
        float m_size = 1.0f;
        int m_experience = 1;
        float m_moveTime = 0.0f; // ダッシュ周期内の経過秒数。

        public:
		CEnemy();
		~CEnemy();

		void Init()override;
		void Update()override;
		void Draw(Camera* cam) override;
		void Uninit()override;

		void Onland();
        void OnHit(int &damage) override;//弾に当たったときの処理
        void SetVariant(SlimeVariant variant);

		void SetPosition(float x, float y, float z) {
			DirectX::SimpleMath::Vector3 p = { x,y,z };
			m_Position = p;
		}

       Collision::Sphere GetCollisionSphere() override {
            return { m_Position, 10.0f * m_size }; // 大型・小型の見た目に判定も合わせる。
       }



};

