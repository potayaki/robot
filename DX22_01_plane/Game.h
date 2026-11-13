/// @file Game.h
/// @brief シーン、オブジェクトの生存期間、更新順序、描画順序の管理。

#pragma once
#include <iostream>
#include <memory>
#include "Object.h"
#include "TestCube.h"
#ifdef _DEBUG
#include "CollisionDebugDraw.h"
#endif
#include "TestPlane.h"
#include "TestModel.h"
#include "Renderer.h"
#include "SkyEnvironment.h"
#include "LowHealthGlitch.h"
#include "TitleScene.h"
#include "Stage1Scene.h"
#include "ResultScene.h"
#include "TestCube.h"

enum SceneName {
    TITLE,
    STAGE1,
    RESULT
};

class CPlayer;

class Game {
private:
    static void FlushObjects(); // 更新終了後に、削除・追加待ちのオブジェクトを反映する。
    /// ゲームインスタンス
    static Game* m_instance;
    /// シーン
    Scene* m_scene;
    /// 次フレームの更新前に反映するシーン遷移先。
    SceneName m_nextScene = TITLE;
    /// UI・オブジェクトの実行中にシーンを破棄しないための予約フラグ。
    bool m_sceneChangePending = false;
    /// シーン読み込みにかかった時間を次フレームの生存時間に加算しない。
    bool m_skipNextElapsed = false;
    /// 死亡時に確定した今回のプレイ記録。
    RunResult m_result;
    /// 記録を確定済みで、死亡演出を再生しているか。
    bool m_gameOver = false;
    /// 死亡演出を再生した経過秒数。
    float m_deathElapsed = 0.0f;
    void ApplySceneChange();
    void CaptureGameOver(CPlayer* player);
    // カメラ
    /// 通常の追従視点とデバッグ用の視点を管理するカメラ。
    Camera m_camera;

    /// HP低下時に画面へ加えるノイズ演出。
    LowHealthGlitch m_lowHealthGlitch;
    /// 昼夜・天候と、それに合わせた空・照明。
    SkyEnvironment m_sky;

    // テストオブジェクト
    /// 地面表示の確認用モデル。
    TestPlane m_plane;

    /// このオブジェクトの見た目を描画するモデル。
    TestModel m_model;

    /// オブジェクトのリスト
    std::vector<std::unique_ptr<Object>> m_objects;
    /// 追加待ちリスト
    std::vector<std::unique_ptr<Object>> m_addObjects;

    /// オブジェクトのリスト
    std::vector<std::unique_ptr<Object>> m_uis;
    /// 追加待ちリスト
    std::vector<std::unique_ptr<Object>> m_addUIs;

#ifdef _DEBUG
    /// 当たり判定を線や球で表示するデバッグ描画処理。
    CollisionDebugDraw m_collisionDebug;
    /// 当たり判定のデバッグ表示を有効にするか。
    bool m_showCollision = false;
#endif

public:
    Game(); // コンストラクタ
    ~Game(); // デストラクタ

    /// 一時停止を除いた実測の経過秒数。
    double m_gameTime = 0.0;

    /// 使用前の状態と必要な資源を初期化する。
    static void Init(); // 初期化
    /// 経過時間と入力に合わせて状態を更新する。
    static void Update(double elapsedSeconds); // 更新
    /// 現在の状態を画面へ描画する。
    static void Draw(); // 描画
    /// 使用した資源を解放して終了する。
    static void Uninit(); // 終了処理

    static Game* GetInstance();

    Camera* GetCamera() {
        return &m_camera;
    }

    void ChangeScene(SceneName sName); //シーンの変更

    void DeleteAllObject(); //オブジェクトをすべて削除

    //オブジェクトを追加する(テンプレート関数
    template <typename T> T* AddObject() {
        T* pt = new T;
        m_instance->m_addObjects.emplace_back(pt);
        pt->Init(); //初期化
        return pt;
    }

    template <typename T> T* AddUI() {
        T* pt = new T;
        m_instance->m_addUIs.emplace_back(pt);
        pt->Init(); //初期化
        return pt;
    }

    //オブジェクトを追加する
    template <typename T> std::vector<T*> GetObjects() {
        std::vector<T*> res;
        for (auto& a : m_instance->m_objects) {
            //dybamiccastで型をチェック
            if (T* derivedObj = dynamic_cast<T*>(a.get())) {
                res.emplace_back(derivedObj);
            }
        }
        return res;
    }

    Scene* GetScene() {
        return m_scene;
    }
};
