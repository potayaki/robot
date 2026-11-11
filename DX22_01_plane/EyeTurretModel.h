/// @file EyeTurretModel.h
/// @brief 目型砲台を構成する球状メッシュと材質の描画。

#pragma once
#include "Camera.h"
#include "EyeTurretShape.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "VertexBuffer.h"

// 目型砲台の見た目だけを担当する。攻撃・寿命・標的の選択はCAutoturretで管理する。
class EyeTurretModel {
private:
    /// このオブジェクトの描画に使用するシェーダー。
    Shader m_shader;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertices;
    /// 描画する頂点の順番を保持するGPUバッファ。
    IndexBuffer m_indices;
    /// 描画で参照するインデックスの数。
    unsigned int m_indexCount = 0;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& center,
              const DirectX::SimpleMath::Quaternion& rotation);
    /// 使用した資源を解放して終了する。
    void Uninit();
};
