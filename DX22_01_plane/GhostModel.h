/// @file GhostModel.h
/// @brief Ghost FBXと支援線・霊弾の共有描画資源。
#pragma once
#include "Camera.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
#include <memory>
class EnemyTint;

class GhostModel {
public:
    static std::shared_ptr<GhostModel> Acquire();
    bool Init();
    /// ワールド座標の中心を基準に、通常サイズへ倍率を掛けて描画する。yawはラジアン。
    void Draw(Camera* camera, DirectX::SimpleMath::Vector3 center, float yaw, float scale = 1.0f,
              const DirectX::SimpleMath::Color& tint = DirectX::SimpleMath::Color(1, 1, 1, 0),
              float hitFlash = 0.0f);
    void DrawBeam(Camera* camera, DirectX::SimpleMath::Vector3 start, DirectX::SimpleMath::Vector3 end,
                  float width, DirectX::SimpleMath::Color color, bool spirit = false);
    /// 支援中の敵の判定球を囲む紫の光を描く。secondsは一時停止で進まないゲーム時刻。
    void DrawSupportAura(Camera* camera, DirectX::SimpleMath::Vector3 center, float radius, double seconds);

private:
    /// モデルを共有しても個体色を次のGhostへ残さないための描画資源。
    std::shared_ptr<EnemyTint> m_tint;
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
    /// 支援線・霊弾・支援オーラで使い回す頂点群。初期化時に最大容量を確保する。
    std::vector<VERTEX_3D> m_vertices;
    /// 支援線・霊弾・支援オーラで使い回す動的バッファ。
    VertexBuffer<VERTEX_3D> m_buffer;
    /// 線を裏側からも表示する描画状態。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
};
