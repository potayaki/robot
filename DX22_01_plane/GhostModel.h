/// @file GhostModel.h
/// @brief Ghost FBXと支援線・霊弾の共有描画資源。
#pragma once
#include "Camera.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
#include <memory>

class GhostModel {
public:
    static std::shared_ptr<GhostModel> Acquire();
    bool Init();
    void Draw(Camera* camera, DirectX::SimpleMath::Vector3 center, float yaw);
    void DrawBeam(Camera* camera, DirectX::SimpleMath::Vector3 start, DirectX::SimpleMath::Vector3 end,
                  float width, DirectX::SimpleMath::Color color, bool spirit = false);

private:
    /// 全Ghostで共有するFBX頂点とインデックス。
    MeshRenderer m_mesh;
    /// FBXの総インデックス数。
    unsigned int m_indexCount = 0;
    /// 頂点色をそのまま表示するシェーダー。
    Shader m_shader;
    /// 霊弾の四角い板を丸い発光として表示するシェーダー。
    Shader m_spiritShader;
    /// 線とモデルの頂点色を表示するための白い画像。
    Texture m_white;
    /// 1本の線を外側と芯の2枚の板で表示する頂点群。
    std::vector<VERTEX_3D> m_vertices;
    /// 支援線と霊弾で使い回す動的バッファ。
    VertexBuffer<VERTEX_3D> m_buffer;
    /// 線を裏側からも表示する描画状態。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
};
