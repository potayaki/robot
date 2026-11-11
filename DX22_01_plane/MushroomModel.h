/// @file MushroomModel.h
/// @brief キノコモデルの読込と、通常色・予告色の描画。

#pragma once
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "Material.h"
#include <memory>

class MushroomModel {
public:
    /// 使用前の状態と必要な資源を初期化する。
    bool Init();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* camera, DirectX::SimpleMath::Vector3 feet, float yaw, bool red);

private:
    /// キノコモデルの頂点・インデックスを描画する処理。
    MeshRenderer m_mesh;
    /// 通常時の照明付き描画シェーダー。
    Shader m_normalShader;
    /// 自爆の予備動作中に赤く点滅させる描画シェーダー。
    Shader m_redShader;
    /// 材質ごとに分けた描画範囲と使用するインデックスの情報。
    std::vector<SUBSET> m_subsets;
    /// サブセットごとの色と反射特性を保持する材質一覧。
    std::vector<std::unique_ptr<Material>> m_materials;
    /// モデルやUIの表示に使用するテクスチャ。
    Texture m_texture;
};
