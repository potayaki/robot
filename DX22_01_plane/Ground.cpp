#include "Ground.h"
#include"stb_image.h"
using namespace DirectX::SimpleMath;
//=======================================
//初期化処理
//=======================================
void Ground::Init() {

	// 4つの頂点で1枚の平らな床を作る
	m_Vertices.resize(4);

	m_Vertices[0].position = Vector3(-1.0f, 0.0f, 1.0f); // 左奥
	m_Vertices[1].position = Vector3(1.0f, 0.0f, 1.0f); // 右奥
	m_Vertices[2].position = Vector3(-1.0f, 0.0f, -1.0f); // 手前左
	m_Vertices[3].position = Vector3(1.0f, 0.0f, -1.0f); // 手前右

	for (int i = 0; i < 4; i++) {
		m_Vertices[i].color = Color(1.0f, 1.0f, 1.0f, 1.0f);
		m_Vertices[i].normal = Vector3(0.0f, 1.0f, 0.0f); // 真上を向く法線
	}

	// UV座標（床が広いのでテクスチャを10回リピートさせる設定）
	m_Vertices[0].uv = Vector2(0.0f, 0.0f);
	m_Vertices[1].uv = Vector2(10.0f, 0.0f);
	m_Vertices[2].uv = Vector2(0.0f, 10.0f);
	m_Vertices[3].uv = Vector2(10.0f, 10.0f);

	// インデックスデータ (三角形2枚)
	m_Indices.resize(6);
	m_Indices[0] = 0; m_Indices[1] = 1; m_Indices[2] = 2;
	m_Indices[3] = 2; m_Indices[4] = 1; m_Indices[5] = 3;

	// バッファ生成
	m_VertexBuffer.Create(m_Vertices);
	m_IndexBuffer.Create(m_Indices);

	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

	// テクスチャの読み込み（画像パスは環境に合わせてください）
	bool sts = m_Texture.Load("assets/texture/field.jpg");
	assert(sts == true);

	// マテリアルの設定
	m_Material = std::make_unique<Material>();
	MATERIAL mtrl;
	mtrl.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
	mtrl.TextureEnable = true;
	m_Material->Create(mtrl);

	// 初期位置とスケール
	m_Position = Vector3(0.0f, 0.0f, 0.0f);
	m_Scale = Vector3(1.0f, 1.0f, 1.0f);
}

//=======================================
//更新処理
//=======================================
void Ground::Update() {

}

//=======================================
//描画処理
//=======================================
void Ground::Draw(Camera* cam) {
	//カメラを選択する
	cam->SetCamera();

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	// 描画の処理
	ID3D11DeviceContext* devicecontext;
	devicecontext = Renderer::GetDeviceContext();

	// トポロジーをセット（プリミティブタイプ）
	devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Shader.SetGPU();
	m_VertexBuffer.SetGPU();
	m_IndexBuffer.SetGPU();
	m_Texture.SetGPU();
	m_Material->SetGPU();

	devicecontext->DrawIndexed(
		m_Indices.size(),	// 描画するインデックス数
		0,					// 最初のインデックスバッファの位置
		0);
}

//=======================================
//終了処理
//=======================================
void Ground::Uninit() {

}

std::vector<VERTEX_3D> Ground::GetVertices() {
	std::vector<VERTEX_3D> res;
	res.resize(m_Vertices.size());

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);

	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);

	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx = s * r * t;

	for (int i = 0; i < m_Vertices.size(); i++) {
		res[i].position = Vector3::Transform(m_Vertices[i].position, worldmtx);

		res[i].normal = Vector3::Transform(m_Vertices[i].normal, worldmtx);

		res[i].color = m_Vertices[i].color;
		res[i].uv = m_Vertices[i].uv;
	}

	return res;
}
