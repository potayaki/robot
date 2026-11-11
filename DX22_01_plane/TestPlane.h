/// @file TestPlane.h
/// @brief 平面モデルの頂点作成と描画。

#pragma once

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"

//-----------------------------------------------------------------------------
//TestPlaneクラス
//-----------------------------------------------------------------------------
class TestPlane {
    // SRT情報（姿勢情報）
    /// モデルまたはオブジェクトの位置。
    DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    /// モデルまたは部品の回転姿勢。
    DirectX::SimpleMath::Vector3 m_rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    /// モデルの拡大縮小倍率。
    DirectX::SimpleMath::Vector3 m_scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

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
    /// シェーダー
    Shader m_shader;
    /// モデルやUIの表示に使用するテクスチャ。
    Texture m_texture;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam);
    /// 使用した資源を解放して終了する。
    void Uninit();
};