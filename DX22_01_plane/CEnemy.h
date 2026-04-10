#pragma once
#include "TestCube.h"	
#include"TestModel.h"
#include "Object.h"
class CEnemy :
    public Object
{

    private:
        //“G‚Ì‘Ì
		TestCube* m_body;

		bool IsGrounded = false;//’…‚¢‚Ä‚¢‚½
        public:
		CEnemy();
		~CEnemy();

		void Init()override;
		void Update()override;
		void Draw(Camera* cam) override;
		void Uninit()override;

		void Onland();

		void SetPosition(float x, float y, float z) {
			DirectX::SimpleMath::Vector3 p = { x,y,z };
			m_Position = p;
		}

};

