/// @file Texture2D.h
/// @brief 画面上へ配置するテクスチャ付き2Dポリゴン。

#pragma once
#include "Object.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include "Material.h"

//-----------------------------------------------------------------------------
// Texture2Dクラス
//-----------------------------------------------------------------------------
class Texture2D : public Object {
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

    // 描画の為の情報（見た目に関わる部分）
    /// テクスチャ
    Texture m_texture;
    /// マテリアル
    std::unique_ptr<Material> m_material;

    // UV座標の情報
    /// 横方向の画像コマの選択位置。
    float m_numU = 1;
    /// 縦方向の画像コマの選択位置。
    float m_numV = 1;
    /// テクスチャを横方向に分割するコマ数。
    float m_splitX = 1;
    /// テクスチャを縦方向に分割するコマ数。
    float m_splitY = 1;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam);
    /// 使用した資源を解放して終了する。
    void Uninit();

    // テクスチャを指定
    void SetTexture(const char* imgname);

    // 位置を指定
    void SetPosition(const float& x, const float& y, const float& z);
    void SetPosition(const DirectX::SimpleMath::Vector3& pos);

    // 角度を指定
    void SetRotation(const float& x, const float& y, const float& z);
    void SetRotation(const DirectX::SimpleMath::Vector3& rot);

    // 大きさを指定
    void SetScale(const float& x, const float& y, const float& z);
    void SetScale(const DirectX::SimpleMath::Vector3& scl);

    // UV座標を指定
    void SetUV(const float& nu, const float& nv, const float& sx, const float& sy);
};
