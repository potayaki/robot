/// @file CParticle.h
/// @brief 火花などの粒子の移動、寿命、共有描画資源。

#pragma once
/*
パーティクルのクラス
*/
#include "Object.h"
#include "TestCube.h"

#include "Texture.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Material.h"

#include "PoolManager.h"

enum ParticleType {
    Test,
    Spark, // 火花
};

class CParticle : public Object {
private:
    /// 破片粒子に加わる重力加速度（ワールド単位/秒²）。
    float m_gravity = 72.0f;

    /// 破片粒子が回転する速さ（ラジアン/秒）。
    float m_spinSpeed = 6.0f;

    /// 基準の1フレームで火花のサイズに掛ける縮小率。
    float m_shrinkPerReferenceFrame = 0.95f;

    /// 旧フレーム単位の縮小率を秒単位へ換算する基準FPS。
    float m_referenceFramesPerSecond = 60.0f;

    /// キャラクターや弾の本体を描画するモデル。
    static TestCube* m_body;
    /// Initを呼んだ粒子の累計数。共有描画資源の生成はPreloadで一度だけ行う。
    static int m_instanceCount;

    //Spark用のパーティクルの情報
    /// 火花粒子で共有する画像。
    static Texture* m_sparkTexture;
    /// 火花粒子で共有する描画シェーダー。
    static Shader* m_sparkShader;
    /// 描画頂点を保持するGPUバッファ。
    static VertexBuffer<VERTEX_3D>* m_sparkVB;
    /// 描画する頂点の順番を保持するGPUバッファ。
    static IndexBuffer* m_sparkIB;
    /// 火花粒子で共有する材質。
    static Material* m_sparkMaterial;

    /// 飛んでいく方向とスピード
    DirectX::SimpleMath::Vector3 m_velocity;
    /// 残りの寿命（秒）。
    float m_life = 1.0f;
    /// パーティクルの種類
    ParticleType m_type = Test;

    /// パーティクルの色
    DirectX::SimpleMath::Color m_color;

    /// パーティクルがアクティブかどうか
    bool m_isActive;

public:
    CParticle();
    ~CParticle();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // 初速は距離/秒、寿命は秒で指定する。
    void SetVelocity(DirectX::SimpleMath::Vector3 vel) {
        m_velocity = vel;
    }

    void SetLife(float life) {
        m_life = life;
    }

    void SetType(ParticleType type) {
        m_type = type;
    }

    void SetColor(DirectX::SimpleMath::Color color) {
        m_color = color;
    }

    bool GetActive() const {
        return m_isActive;
    }

    void SetActive(bool isActive) {
        m_isActive = isActive;
    }

    static void Preload();
};

using ParticleManager = PoolManager<CParticle, 5000>; // 最大5000個の粒子を再利用する。
