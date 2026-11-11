/// @file CollisionDebugDraw.h
/// @brief 当たり判定をテクスチャなしの線で表示するデバッグ描画。

#pragma once
#ifdef _DEBUG
#include "Collision.h"
#include "Camera.h"
#include "Shader.h"
#include "VertexBuffer.h"

// 判定の中心・半径をそのまま表示するための、テクスチャを使わないデバッグ描画。
class CollisionDebugDraw {
public:
    struct Sphere {
        /// デバッグ描画する当たり判定の球。
        Collision::Sphere shape;
        /// 描画色。RGBAの各成分を保持する。
        DirectX::SimpleMath::Color color;
    };

    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera, const std::vector<Sphere>& spheres);
    /// 使用した資源を解放して終了する。
    void Uninit();

private:
    /// このオブジェクトの描画に使用するシェーダー。
    Shader m_shader;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertices;
    /// 色や材質の情報をGPUへ渡す定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_materialBuffer;
    /// 深度テストと深度書き込みの設定。
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth;
    /// カリングや塗りつぶし方法を指定する描画設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
    /// 描画対象となる頂点の数。
    unsigned m_vertexCount = 0;
    /// 必要な描画資源の準備が完了しているか。
    bool m_ready = false;
};
#endif
