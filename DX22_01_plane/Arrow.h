/// @file Arrow.h
/// @brief ゴルフ操作で方向と強さを示す矢印モデル。

#pragma once
#include "Object.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"

//-----------------------------------------------------------------------------
// Arrowクラス
//-----------------------------------------------------------------------------
class Arrow : public Object {
private:
    // 描画の為の情報（メッシュに関わる情報）
    /// 頂点バッファ・インデックスバッファ・インデックス数
    MeshRenderer m_meshRenderer;

    // 描画の為の情報（見た目に関わる部分）
    /// サブセットごとの色と反射特性を保持する材質一覧。
    std::vector<std::unique_ptr<Material>> m_materials;
    /// 材質ごとに分けた描画範囲と使用するインデックスの情報。
    std::vector<SUBSET> m_subsets;
    /// テクスチャ
    std::vector<std::unique_ptr<Texture>> m_textures;

    /// 0:非表示・1:方向選択・2:パワー選択
    int m_state = 0;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam);
    /// 使用した資源を解放して終了する。
    void Uninit();

    // 状態の設定
    void SetState(int s);

    // 矢印のベクトルを取得
    DirectX::SimpleMath::Vector3 GetVector();
};
