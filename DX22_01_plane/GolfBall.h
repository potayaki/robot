/// @file GolfBall.h
/// @brief ゴルフ用ボールの移動、接地、衝突、描画。

//free3Dから
#pragma once

#include "Object.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "StaticMesh.h"
#include "utility.h"
#include "Material.h"

class GolfBall : public Object {
private:
    //速度
    /// 位置を変化させる速度ベクトル（ワールド単位/秒）。
    DirectX::SimpleMath::Vector3 m_velocity = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    //加速度
    /// 速度を毎秒変化させる加速度ベクトル。
    DirectX::SimpleMath::Vector3 m_acceleration = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
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

    /// 状態：０：物理挙動、１：停止、２：カップイン
    int m_state = 0;
    /// 静止カウント
    int m_stopCount = 0;

public:
    /// 使用前の状態と必要な資源を初期化する。
    void Init();
    /// 経過時間と入力に合わせて状態を更新する。
    void Update();
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam);
    /// 使用した資源を解放して終了する。
    void Uninit();

    void SetState(int s) {
        m_state = s;
    }

    int Getstate() {
        return m_state;
    }

    void shot(DirectX::SimpleMath::Vector3 v) {
        m_velocity = v;
    }

    void SetColor(DirectX::SimpleMath::Color col);
};
