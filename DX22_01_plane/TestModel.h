/// @file TestModel.h
/// @brief ゲーム内で使用する静的モデルの読込とサブセット描画。

#pragma once

#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"
#include "Object.h"

class TestModel : public Object {
private:
    // 描画の為の情報（メッシュに関わる情報）
    /// 頂点バッファ・インデックスバッファ・インデックス数
    MeshRenderer m_meshRenderer;

    // 描画の為の情報（見た目に関わる部分）
    /// サブセットごとの色と反射特性を保持する材質一覧。
    std::vector<std::unique_ptr<Material>> m_materiales;
    /// 材質ごとに分けた描画範囲と使用するインデックスの情報。
    std::vector<SUBSET> m_subsets;

    /// テクスチャ
    std::vector<std::unique_ptr<Texture>> m_textures;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // モデル、FBX が参照するテクスチャのフォルダ、必要なら明示的に割り当てる画像
    void Load(std::string modelFile, std::string texDirectory, std::string overrideTextureFile = "",
              bool smoothNormals = false);
};
