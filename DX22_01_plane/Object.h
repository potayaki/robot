/// @file Object.h
/// @brief 位置・回転・倍率と、Init・Update・Draw・Uninitを持つ共通基底。

#pragma once
/*
オブジェクトの基底クラス
ゲーム内のすべてのオブジェクト（プレイヤー、敵、弾、地面など）が継承する共通クラス。
位置・回転・スケールなどの SRT（姿勢情報）と、
描画・更新・破棄などの基本インターフェースを提供する。
*/
#include "Camera.h"
#include "GameTime.h"
#include "Shader.h"
#include "Collision.h"

class Object {
protected:
    //-----------------------------
    //SRT（姿勢情報）
    //-----------------------------
    /// オブジェクトの位置（ワールド座標）。
    DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    /// 各軸まわりの回転角（ラジアン）。
    DirectX::SimpleMath::Vector3 m_rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    /// 各軸方向の拡大縮小倍率。1が元のサイズ。
    DirectX::SimpleMath::Vector3 m_scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

    //-----------------------------
    // 描画の為の情報（見た目に関わる部分）
    //-----------------------------
    /// シェーダー
    Shader m_shader;

    //-----------------------------
    //オブジェクト状態管理
    //-----------------------------
    /// オブジェクトが死んでいるかどうかのフラグ
    bool m_dead = false;
    /// 更新・描画の対象として有効になっているか。
    bool m_active = true;

public:
    virtual ~Object(); //仮想デストラクタ

    //-----------------------------
    //派生クラスで実装する純粋仮想関数
    //-----------------------------
    /// 使用前の状態と必要な資源を初期化する。
    virtual void Init() = 0;
    /// 経過時間と入力に合わせて状態を更新する。
    virtual void Update() = 0;
    /// 現在の状態を画面へ描画する。
    virtual void Draw(Camera* cam) = 0;
    /// 使用した資源を解放して終了する。
    virtual void Uninit() = 0;

    // 位置の取得
    DirectX::SimpleMath::Vector3 GetPosition() const {
        return m_position;
    }

    DirectX::SimpleMath::Vector3 GetScale() const {
        return m_scale;
    }

    //-----------------------------
    //Getter/Setter
    //-----------------------------
    void SetRotation(DirectX::SimpleMath::Vector3 rot) {
        m_rotation = rot;
    }

    Object* SetPosition(float x, float y, float z) {
        DirectX::SimpleMath::Vector3 p = {x, y, z};
        m_position = p;
        return this;
    }

    void SetPosition(DirectX::SimpleMath::Vector3 pos) {
        m_position = pos;
    }

    void SetScale(DirectX::SimpleMath::Vector3 scl) {
        m_scale = scl;
    }

    Object* SetScale(float x, float y, float z) {
        m_scale.x = x;
        m_scale.y = y;
        m_scale.z = z;
        return this;
    }

    Object* SetScale(float size) {
        m_scale.x = size;
        m_scale.y = size;
        m_scale.z = size;
        return this;
    }

    //デフォルトでは半径0（当たり判定なし）の球を返すようにしておく
    virtual Collision::Sphere GetCollisionSphere() {
        return {m_position, 0.0f};
    }

    //-----------------------------
    //オブジェクトの生存管理
    //-----------------------------
    void Destroy() {
        m_dead = true;
    } //オブジェクトを破棄する

    bool IsDead() const {
        return m_dead;
    } //オブジェクトが死んでいるかどうかを返す

    void SetActive(bool active) {
        m_active = active; //オブジェクトのアクティブ状態を設定する
    }

    bool GetActive() const {
        return m_active;
    }
};
