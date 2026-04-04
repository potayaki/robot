#pragma once
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Camera.h"
#include "Shader.h"
#include"Texture.h"
#include"Object.h"
#include "Material.h"
//-----------------------------------------------------------------------------
//TestPlaneクラス
//-----------------------------------------------------------------------------
class Ground : public Object
{
	

	// 頂点データ
	std::vector<VERTEX_3D> m_Vertices;

	//インデックスデータ
	std::vector<unsigned int> m_Indices;

	// 描画の為の情報（メッシュに関わる情報）
	IndexBuffer	 m_IndexBuffer; // インデックスバッファ
	VertexBuffer<VERTEX_3D>	m_VertexBuffer; // 頂点バッファ

	// 描画の為の情報（見た目に関わる部分）
	
	Texture m_Texture;
	std::unique_ptr<Material>m_Material;

	int m_SizeX = 0;
	int m_SizeZ = 0;
public:
	void Init()override;
	void Update()override;
	void Draw(Camera* cam)override;
	void Uninit()override;
	
	std::vector<VERTEX_3D> GetVertices();
};

