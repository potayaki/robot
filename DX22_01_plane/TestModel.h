/// @file TestModel.h
/// @brief ゲーム内で使用する静的モデルの読込とサブセット描画。

#pragma once

#include <memory>
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"
#include "Object.h"

class TestModel : public Object {
private:
    struct Resources;
    /// 同じ読込設定を使う個体間で共有するメッシュ・材質・テクスチャ。
    std::shared_ptr<Resources> m_resources;

public:
    /// 同じモデルを使うオブジェクト同士で、メッシュ・材質・テクスチャを共有する。
    /// @param cachedModel モデルの種類ごとに用意する弱参照。最後の利用者がいなくなればモデルは解放される。
    /// @param modelFile この弱参照に対応するモデルファイルのパス。
    /// @param textureDirectory モデルが参照するテクスチャの検索先。
    static std::shared_ptr<TestModel> AcquireShared(std::weak_ptr<TestModel>& cachedModel,
                                                    const char* modelFile, const char* textureDirectory);
    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// このモデル自身が保持する位置・回転・倍率で描画する。
    void Draw(Camera* camera) override;
    /// 現在の姿勢で個体色を適用する。tintのAは着色率で、透明度は変更しない。
    void Draw(Camera* camera, const DirectX::SimpleMath::Color& tint);
    /// 同じモデルを共有する弾などが、自分の位置・回転・倍率で描画するときに使う。
    /// @param worldPosition モデルの原点を配置するワールド座標。
    /// @param rotationRadians X 軸のピッチ、Y 軸のヨー、Z 軸のロール（ラジアン）。
    /// @param modelScale モデルのローカル X・Y・Z 軸に沿った拡大率。
    void Draw(Camera* camera, const DirectX::SimpleMath::Vector3& worldPosition,
              const DirectX::SimpleMath::Vector3& rotationRadians,
              const DirectX::SimpleMath::Vector3& modelScale,
              const DirectX::SimpleMath::Color& tint = DirectX::SimpleMath::Color(1, 1, 1, 0));
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // モデル、FBX が参照するテクスチャのフォルダ、必要なら明示的に割り当てる画像
    void Load(std::string modelFile, std::string texDirectory, std::string overrideTextureFile = "",
              bool smoothNormals = false);
};
