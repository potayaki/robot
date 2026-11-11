/// @file Ground.h
/// @brief 地形グリッドの作成とテクスチャ付きの地面描画。

#pragma once
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Object.h"
#include "Material.h"

//-----------------------------------------------------------------------------
//Groundクラス　床
//-----------------------------------------------------------------------------
class Ground : public Object {
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

    // 描画の為の情報（見た目に関わる部分）

    /// モデルやUIの表示に使用するテクスチャ。
    Texture m_texture;
    /// 描画時に使用する色・反射・テクスチャ有効状態。
    std::unique_ptr<Material> m_material;

    /// 地形グリッドのX方向の分割数。
    int m_sizeX = 0;
    /// 地形グリッドのZ方向の分割数。
    int m_sizeZ = 0;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    std::vector<VERTEX_3D> GetVertices();
};
