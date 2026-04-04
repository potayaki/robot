#pragma once

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include"Texture.h"
#include"Object.h"
//-----------------------------------------------------------------------------
//TestCubeクラス
//-----------------------------------------------------------------------------
class TestCube:public Object {
private:

	// 頂点データ
	std::vector<VERTEX_3D> m_Vertices;

	//インデックスデータ
	std::vector<unsigned int> m_Indices;

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ

	
	Texture m_texture;

public:
	void Init()override;
	void Update()override;
	void Draw(Camera* cam)override;
	void Uninit()override;
	void SetMaterial(DirectX::SimpleMath::Color col);
};
