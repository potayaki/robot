#pragma once

#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"
#include "Object.h"
class TestModel:public Object {
private:
	

	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数

	// 描画の為の情報（見た目に関わる部分）
	std::vector<std::unique_ptr<Material>> m_Materiales;
	std::vector<SUBSET> m_subsets;
	
	std::vector<std::unique_ptr<Texture>> m_Textures; // テクスチャ

public:
	void Init()override;
	void Update()override;
	void Draw(Camera* cam)override;
	void Uninit()override;

	// モデル、FBX が参照するテクスチャのフォルダ、必要なら明示的に割り当てる画像
    void Load(std::string modelFile, std::string texDirectory,
        std::string overrideTextureFile = "", bool smoothNormals = false);
};
