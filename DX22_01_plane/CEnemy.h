#pragma once
#include "TestCube.h"	
#include"TestModel.h"
#include "Object.h"
class CEnemy :
    public Object
{

    private:
        //敵の体
		TestCube* m_body;

        int hp = 3;//体力

		bool IsGrounded = false;//着いていた
        public:
		CEnemy();
		~CEnemy();

		void Init()override;
		void Update()override;
		void Draw(Camera* cam) override;
		void Uninit()override;

		void Onland();
        void OnHit(int &damage);//弾に当たったときの処理

		void SetPosition(float x, float y, float z) {
			DirectX::SimpleMath::Vector3 p = { x,y,z };
			m_Position = p;
		}

};

