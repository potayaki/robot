#include	"TestModel.h"

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void TestModel::Init()
{
	// シェーダオブジェクト生成
	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    m_Scale = Vector3(1.0f,1.0f,1.0f);
    m_Position = Vector3(0.0f, 0.0f, 0.0f);
    m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
    
}

//=======================================
//更新処理
//=======================================
void TestModel::Update()
{

}

//=======================================
//描画処理
//=======================================
void TestModel::Draw(Camera* cam)
{
	//カメラを選択する
	cam->SetCamera();

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット
	m_MeshRenderer.BeforeDraw();

	//マテリアル数分ループ 
	for (int i = 0; i < m_subsets.size(); i++)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックを使用)
		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum,    // 描画するインデックス数
			m_subsets[i].IndexBase,   // 最初のインデックスバッファの位置
			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	}
}

void TestModel::Load(std::string modelFile, std::string texFile) {

    // メッシュ読み込み
    StaticMesh staticmesh;

    // 引数でもらったファイル名とフォルダ名を使ってMeshを読み込む
    staticmesh.Load(modelFile,texFile);
    m_MeshRenderer.Init(staticmesh);

    // サブセット情報・テクスチャ情報取得
    m_subsets = staticmesh.GetSubsets();
    m_Textures = staticmesh.GetTextures();

    // マテリアル情報取得	
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();

    // 念のため配列をクリアしておく
    m_Materiales.clear();

    // マテリアル数分ループ
    for (int i = 0; i < materials.size(); i++) {
        std::unique_ptr<Material> m = std::make_unique<Material>();
        m->Create(materials[i]);
        m_Materiales.push_back(std::move(m));
    }
}

//=======================================
//終了処理
//=======================================
void TestModel::Uninit()
{

}

