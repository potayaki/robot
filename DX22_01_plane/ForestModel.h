/// @file ForestModel.h
/// @brief 材質色を持つ森のFBXを読み込み、足元と高さを指定して繰り返し描く。
#pragma once
#include "Camera.h"
#include "Material.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
#include <memory>

class ForestModel {
public:
    /// FBXを高さ1・底面中央原点へ正規化する。画像指定時は元のUVへ割り当てる。
    bool Load(const char* path, const char* texturePath = nullptr);
    /// feetは足元のワールド座標、yawはラジアン、heightはワールド単位の高さ。
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& feet, float yaw, float height);

private:
    /// 同じ素材の全配置で共用するGPUメッシュ。
    MeshRenderer m_mesh;
    /// マテリアル色と頂点色を乗算した頂点に、照明を適用するシェーダー。
    Shader m_shader;
    /// 焼き込んだ頂点色に乗算する白色材質。画像指定時だけテクスチャを有効にする。
    std::unique_ptr<Material> m_material;
    /// テントなどで元のUVに割り当てる任意の色画像。
    Texture m_texture;
    /// 草や葉を裏面から見ても表示する描画設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
    /// 全メッシュをまとめたインデックス数。0なら読み込み未完了。
    unsigned int m_indexCount = 0;
};
