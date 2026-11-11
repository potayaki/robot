/// @file MeshRenderer.h
/// @brief メッシュの頂点・インデックスバッファを使った描画。

#pragma once
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Mesh.h"

class MeshRenderer {
protected:
    /// 頂点バッファ
    VertexBuffer<VERTEX_3D> m_vertexBuffer;
    /// インデックスバッファ
    IndexBuffer m_indexBuffer;
    /// インデックス数
    int m_indexNum = 0;

public:
    /// 使用前の状態と必要な資源を初期化する。
    virtual void Init(Mesh& mesh) {
        m_vertexBuffer.Create(mesh.GetVertices());
        m_indexBuffer.Create(mesh.GetIndices());
        m_indexNum = static_cast<int>(mesh.GetIndices().size());
    }

    // 描画前処理
    virtual void BeforeDraw() {
        ID3D11DeviceContext* devicecontext = Renderer::GetDeviceContext();

        // トポロジーをセット（旧プリミティブタイプ）
        devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_vertexBuffer.SetGPU(); // 頂点バッファをセット
        m_indexBuffer.SetGPU(); // インデックスバッファをセット
    }

    // サブセット描画
    virtual void DrawSubset(unsigned int indexnum, unsigned int baseindex, unsigned int basevertexindex) {
        Renderer::GetDeviceContext()->DrawIndexed(indexnum, // 描画するインデックス数（面数×３）
                                                  baseindex, // 最初のインデックスバッファの位置
                                                  basevertexindex); // 頂点バッファの最初から使う
    }

    // 描画
    /// 現在の状態を画面へ描画する。
    virtual void Draw() {
        BeforeDraw(); // 描画前処理

        Renderer::GetDeviceContext()->DrawIndexed(m_indexNum, // 描画するインデックス数（面数×３）
                                                  0, // 最初のインデックスバッファの位置
                                                  0); // 頂点バッファの最初から使う
    }
};
