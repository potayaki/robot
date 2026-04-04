//free3Dから
#pragma once

#include "Object.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"


class GolfBall :public Object
{
private:
	//速度
	DirectX::SimpleMath::Vector3 m_Velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f );
	//加速度
	DirectX::SimpleMath::Vector3 m_Acceleration = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数

	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materials;
	std::vector<SUBSET> m_subsets;
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ

	int m_state = 0;//状態：０：物理挙動、１：停止、２：カップイン
	int m_StopCount = 0;//静止カウント
	
public:
	void Init();
	void Update();
	void Draw(Camera* cam);
	void Uninit();

	
	void SetState(int s) { m_state = s; }
	int Getstate() { return m_state; }
	void shot(DirectX::SimpleMath::Vector3 v) { m_Velocity = v; }
	void SetColor(DirectX::SimpleMath::Color col);
};

