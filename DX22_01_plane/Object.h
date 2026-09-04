#pragma once
#include "Camera.h"
#include "Shader.h"
#include"Collision.h"
class Object {
protected:
	// SRT情報（姿勢情報）
	DirectX::SimpleMath::Vector3 m_Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_Rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

	// 描画の為の情報（見た目に関わる部分）
	Shader m_Shader; // シェーダー

    bool m_Dead = false; //オブジェクトが死んでいるかどうかのフラグ
    bool m_Active = true;
public:
	virtual ~Object();//仮想デストラクタ

	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw(Camera* cam) = 0;
	virtual void Uninit() = 0;

	// 位置の取得
	DirectX::SimpleMath::Vector3 GetPosition() const { return m_Position; }
	DirectX::SimpleMath::Vector3 GetScale() const { return m_Scale; }


	void  SetRotation(DirectX::SimpleMath::Vector3 rot) { m_Rotation = rot; }
	Object* SetPosition(float x, float y, float z) {
		DirectX::SimpleMath::Vector3 p = { x,y,z };
		m_Position = p;
		return this;
	}
	void SetPosition(DirectX::SimpleMath::Vector3 pos) {
		m_Position = pos;
	}
	void SetScale(DirectX::SimpleMath::Vector3 scl) { m_Scale = scl; }
	Object* SetScale(float x, float y, float z) { m_Scale.x = x; m_Scale.y = y; m_Scale.z = z; return this; }

    Object* SetScale(float size) { m_Scale.x = size; m_Scale.y = size; m_Scale.z = size; return this; }

    //デフォルトでは半径0（当たり判定なし）の球を返すようにしておく
    virtual Collision::Sphere GetCollisionSphere() {
        return { m_Position, 0.0f };
    }

    void Destroy() { m_Dead = true; } //オブジェクトを破棄する
    bool IsDead() const { return m_Dead; } //オブジェクトが死んでいるかどうかを返す

    void SetActive(bool active) {
        m_Active = active;
    }

    bool GetActive() const {
        return m_Active;
    }

};

