#pragma once
#include "Object.h"
#include"TestCube.h"
#include"GolfBall.h"
#include"Bezier.h"
class CPlayer : public Object
{
private:
	static const int MaxRocket = 5;//最大弾数
	TestCube* m_body;//body

    float  m_BulletTime = 60 * 0.1;//60フレームで０．５秒間隔で打つ用の保存
    float m_currentBulletTime = 0.0f;//次打てるまでの秒数0なら撃てる

    float m_MissileTime = 60 * 2;//60フレームで０．５秒間隔で打つ用の保存
    float m_currentMissileTime = 0.0f;//次打てるまでの秒数0なら撃てる

	DirectX::SimpleMath::Vector3 m_velocity = { 0,0,0 };
	bool IsGrounded = false;//地面についているか


	
public:
	CPlayer();
	~CPlayer();

	void Init()override;
	void Update()override;
	void Draw(Camera* cam) override;
	void Uninit()override;

    //WASD、SPACEの挙動
	void Move();
    //床との当たり判定
    void isGrounded();
    //左クリック時にBulletを作り
    void StartBullet();

    void StartMissile();

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

