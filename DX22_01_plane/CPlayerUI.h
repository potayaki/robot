/// @file CPlayerUI.h
/// @brief HPとミサイル待ち時間をリング状に表示するUI。

#pragma once
#include "Object.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

struct UIBuffer {
    /// リングUIの残量（0～1）。
    float Rate;
    /// リングに表示するRGB色。
    DirectX::SimpleMath::Vector3 Color;
};

class CPlayerUI : public Object {
private:
    /// 描画に使用する頂点の配列。
    std::vector<VERTEX_3D> m_vertices;
    /// 三角形を構成する頂点番号の配列。
    std::vector<unsigned int> m_indices;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertexBuffer;
    /// 描画する頂点の順番を保持するGPUバッファ。
    IndexBuffer m_indexBuffer;

    /// HPリングの描画に使用する画像。
    Texture m_textureHP;
    /// ミサイルの待ち時間リングに使用する画像。
    Texture m_textureMissile;

    /// このオブジェクトの描画に使用するシェーダー。
    Shader m_shader;

    /// GPUに値を送るためのバッファ
    ID3D11Buffer* m_pUIBuffer = nullptr;

    /// 現在HPを最大HPで割った割合（0～1）。
    float m_hpRate;
    /// ミサイルの再発射準備が完了した割合（0～1）。
    float m_missileRate;

public:
    CPlayerUI();
    ~CPlayerUI();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // 外から割合を設定するための関数
    void SetHpRate(float rate) {
        m_hpRate = rate;
    }

    void SetMissileRate(float rate) {
        m_missileRate = rate;
    }
};
