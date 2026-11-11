/// @file CMiniMap.h
/// @brief プレイヤーと敵の位置を表示するミニマップ。

#pragma once
#include "Object.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"

class CMiniMap : public Object {
private:
    /// 描画に使用する頂点の配列。
    std::vector<VERTEX_3D> m_vertices;
    /// 三角形を構成する頂点番号の配列。
    std::vector<unsigned int> m_indices;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertexBuffer;
    /// 描画する頂点の順番を保持するGPUバッファ。
    IndexBuffer m_indexBuffer;

    /// ：背景（枠）用
    VertexBuffer<VERTEX_3D> m_vertexBufferBg;
    /// ：敵（赤色）用
    VertexBuffer<VERTEX_3D> m_vertexBufferEnemy;
    /// プレイヤーと黒い内側背景に使う白色の頂点
    VertexBuffer<VERTEX_3D> m_vertexBufferPlayer;

    /// レーダーの内側背景用
    Texture m_texBackground;
    /// 黒い空でも見える色付きの外枠用
    Texture m_texFrame;
    /// プレイヤー
    Texture m_texPlayer;
    /// 敵の点用
    Texture m_texEnemy;

    /// このオブジェクトの描画に使用するシェーダー。
    Shader m_shader;
    /// 描画時に使用する色・反射・テクスチャ有効状態。
    std::unique_ptr<Material> m_material;

    /// レーダーの探知半径
    float m_radarRange = 500.0f;

public:
    CMiniMap();
    ~CMiniMap();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;
};
