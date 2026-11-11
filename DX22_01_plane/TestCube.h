/// @file TestCube.h
/// @brief 単純な立方体モデルの頂点作成と描画。

#pragma once

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "Object.h"
#include "string"
#include <memory>
#include "Material.h"

//-----------------------------------------------------------------------------
//TestCubeクラス
//-----------------------------------------------------------------------------
class TestCube : public Object {
private:
    // 頂点データ
    /// 描画に使用する頂点の配列。
    std::vector<VERTEX_3D> m_vertices;

    //インデックスデータ
    /// 三角形を構成する頂点番号の配列。
    std::vector<unsigned int> m_indices;

    // 描画の為の情報（メッシュに関わる情報）
    /// インデックスバッファ
    IndexBuffer m_indexBuffer;
    /// 頂点バッファ
    VertexBuffer<VERTEX_3D> m_vertexBuffer;

    /// モデルやUIの表示に使用するテクスチャ。
    Texture m_texture;

    /// マテリアル情報
    std::unique_ptr<Material> m_material;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
    void SetMaterial(DirectX::SimpleMath::Color col);
    void SetTexture(std::string texture);
};
