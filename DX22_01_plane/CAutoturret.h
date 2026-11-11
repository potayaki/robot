/// @file CAutoturret.h
/// @brief プレイヤーに追従する目型砲台の照準・攻撃・レーザー表示。

#pragma once
#include "Object.h"
#include "EyeTurretModel.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "PoolManager.h"

class CAutoturret : public Object {
private:
    /// 白目・虹彩・瞳を持つ立体の目。
    EyeTurretModel m_eye;
    /// 毎フレームの姿勢を保存し、滑らかに回す。
    DirectX::SimpleMath::Quaternion m_eyeRotation{};
    /// 攻撃のクールダウン時間
    float m_attackCooldown = 1.0f / 6.0f;
    /// 攻撃範囲の半径
    float m_radius = 500.0f;
    /// 攻撃のダメージ量
    int m_damage = 1;

    // 有効/無効はObjectのm_Activeを使い、プールと寿命の判定を一致させる。
    /// 秒。
    float m_attackInterval = 0.45f;

    // 攻撃時の2点だけを保存する。敵が消えても、破棄された敵のポインタは参照しない。
    /// 実測時間で0.2秒表示。
    static constexpr float LaserDurationSeconds = 0.2f;
    /// 最後に発射した光線の始点（ワールド座標）。
    DirectX::SimpleMath::Vector3 m_laserStart{};
    /// 最後に発射した光線の終点（ワールド座標）。
    DirectX::SimpleMath::Vector3 m_laserEnd{};
    /// 光線を表示する残り秒数。
    float m_laserTime = 0.0f;

    // 細い芯と、その周囲の淡い光の2枚の板でレーザーを表現する。
    // 初期化時に作成し、攻撃や描画のたびにGPUのバッファを作り直さない。
    /// 相棒砲台の光線を描くシェーダー。
    Shader m_laserShader;
    /// 光線の芯と周辺の発光に使用する画像。
    Texture m_laserTexture;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_laserBuffer;
    /// 光線の始点・終点から作る描画頂点。
    std::vector<VERTEX_3D> m_laserVertices;
    /// 光線を両面から表示するための描画設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_laserRasterizer;
    /// 光線の描画資源を作成できたか。
    bool m_laserReady = false;

    void InitLaser();

    DirectX::SimpleMath::Vector3 GetEyeCenter() const {
        return m_position + DirectX::SimpleMath::Vector3(0.0f, EyeTurretShape::Height, 0.0f);
    }

public:
    CAutoturret();
    ~CAutoturret();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override; // 初期化処理
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override; // 更新処理
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override; // 描画処理
    void DrawLaser(Camera* cam); // 地形・敵の描画後に呼び、光線だけを重ねる。
    /// 使用した資源を解放して終了する。
    void Uninit() override; // 終了処理(モデルの破棄)

    float GetAttackCooldown() const {
        return (std::max)(0.0f, m_attackCooldown);
    }

    // 攻撃のクールダウンを秒で設定する関数
    void SetAttackCooldown(float cooldown) {
        m_attackCooldown = cooldown;
    }

    void ConfigureUpgrade(int rank);

    // 当たり判定用の球を返す関数
    Collision::Sphere GetCollisionSphere() override {
        return {GetEyeCenter(), EyeTurretShape::Radius}; // 表示とデバッグ用の球を同じ位置・寸法にする。
    }

    void Spawn(const DirectX::SimpleMath::Vector3& position) {
        m_position = position;
        m_active = true; // 自動砲台をアクティブにする
        m_attackCooldown = 1.0f / 6.0f; // クールダウンをリセット

        // プールから再設置した砲台に、前回の場所のレーザーを残さない。
        m_laserTime = 0;
        m_eyeRotation = DirectX::SimpleMath::Quaternion{}; // 再設置時は正面から照準を開始する。
    }
};

// 生成・更新・描画のすべてで上限1体の同じプールを使う。
using AutoTurretManager = PoolManager<CAutoturret, 1>;
