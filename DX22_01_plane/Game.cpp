#include "Game.h"
#include "Renderer.h"
#include"input.h"


Game* Game::m_instance;//ゲームインスタンス

// コンストラクタ
Game::Game() {
    m_scene = nullptr;
}

// デストラクタ
Game::~Game() {
    delete m_scene;
    DeleteAllObject();//すべてのオブジェクトを削除
}

// 初期化
void Game::Init() {
    //インスタンス作成
    m_instance = new Game;
    // 描画初期化処理
    Renderer::Init();
    Input::Create();

    // カメラ初期化
    m_instance->m_Camera.Init();
    //最初のシーンを読み込む
    m_instance->m_scene = new Stage1Scene;//TODO:最初のシーンをタイトルシーンにする


}

// 更新
void Game::Update() {
    //シーン更新
    Input::Update();
    //シーン更新
    m_instance->m_scene->Update();
    // カメラ更新
    m_instance->m_Camera.Update();



    // テストオブジェクト更新
    for (auto& a : m_instance->m_objects) {
        a->Update();
    }

    for (auto& a : m_instance->m_UIs) {
        a->Update();
    }

    //削除待ちオブジェクトの削除
    for (auto& removeObj : m_instance->m_removeObjects) {
        // 1. 名簿(m_objects)の中に、消したい奴が「まだ生き残っているか」探す
        auto it = std::find_if(m_instance->m_objects.begin(), m_instance->m_objects.end(),
            [removeObj](const std::unique_ptr<Object>& obj) {
                return obj.get() == removeObj;
            });

        // 2. もし名簿にまだ残っていたら（二重削除じゃなければ）正式に消す
        if (it != m_instance->m_objects.end()) {
            removeObj->Uninit(); // 画像などのメモリをお掃除
            m_instance->m_objects.erase(it); // 名簿から削除
        }
        else {
            // m_objects に無かったら m_uiObjects から探して消す↓
            auto ui_it = std::find_if(m_instance->m_UIs.begin(), m_instance->m_UIs.end(),
                [removeObj](const std::unique_ptr<Object>& obj) {
                    return obj.get() == removeObj;
                });

            if (ui_it != m_instance->m_UIs.end()) {
                removeObj->Uninit();
                m_instance->m_UIs.erase(ui_it);
            }
        }

    }
    m_instance->m_removeObjects.clear();

    //追加待ちオブジェクトの追加
    for (auto& addObj : m_instance->m_addObjects) {
        m_instance->m_objects.emplace_back(std::move(addObj));
    }
    m_instance->m_addObjects.clear();

    for (auto& addObj : m_instance->m_addUIs) {
        m_instance->m_UIs.emplace_back(std::move(addObj));
    }
    m_instance->m_addUIs.clear();

}

// 描画
void Game::Draw() {
    // 描画前処理
    Renderer::DrawStart();

    // テストオブジェクト描画
    for (auto& a : m_instance->m_objects) {
        a->Draw(&m_instance->m_Camera);
    }
    if (m_instance->m_scene) {
        m_instance->m_scene->Draw(&m_instance->m_Camera);
    }

    Renderer::SetDepthEnable(false);

    for (auto &a:m_instance->m_UIs) {

        a->Draw(&m_instance->m_Camera);

    }

    Renderer::SetDepthEnable(true);


    // 描画後処理
    Renderer::DrawEnd();
}

// 終了処理
void Game::Uninit() {
    // カメラ終了処理
    m_instance->m_Camera.Uninit();

    // テストオブジェクト終了処理
    m_instance->m_Plane.Uninit();
    m_instance->m_model.Uninit();
    for (auto& a : m_instance->m_objects) {
        a->Uninit();
    }
    for (auto& a : m_instance->m_UIs) {
        a->Uninit();
    }
    
    Input::Release();
    // 描画終了処理
    Renderer::Uninit();

    delete m_instance;
}

Game* Game::GetInstance() {
    return m_instance;
}

//シーンを切り替える
void Game::ChangeScene(SceneName sName) {
    //読み込み済みのシーンがあれば削除
    int score = 0;

    if (m_instance->m_scene != nullptr) {
        //消そうとしているシーンがStage1ならスコアを保存しておく
        /*
        if (Stage1Scene* sObj = dynamic_cast<Stage1Scene*>(m_instance->m_scene)) {
            score = sObj->Getscore();
        }
        */
        delete m_instance->m_scene;
        m_instance->m_scene = nullptr;
    }


    switch (sName) {
        case TITLE:
        m_instance->m_scene = new TitleScene;
        break;
        case STAGE1:
        m_instance->m_scene = new Stage1Scene;
        break;
        case RESULT:
        m_instance->m_scene = new ResultScene;
        dynamic_cast<ResultScene*>(m_instance->m_scene)->Setscore(score);//スコアを設定
        break;
    }
}

void Game::DeleteObject(Object* pt) {
    if (pt == NULL) {
        return;
    }
    m_instance->m_removeObjects.push_back(pt);
}

void Game::DeleteAllObject() {

    for (auto& o : m_instance->m_objects) {
        o->Uninit();
    }
    m_instance->m_objects.clear();
    m_instance->m_objects.shrink_to_fit();
    for (auto& o : m_instance->m_UIs) {
        o->Uninit();
    }
    m_instance->m_UIs.clear();
    m_instance->m_UIs.shrink_to_fit();

}
