#include "Game.h"
#include "Renderer.h"
#include"input.h"
#include"billboard.h"

#include"imgui.h"
#include"imgui_impl_dx11.h"
#include"imgui_impl_win32.h"
#include"Application.h"//GetWindow()を使うために必要

#include"CPlayer.h"
#include"CPlayerUI.h"
#include"PoolManager.h"
#include"CParticle.h"
#include"CEnemy.h"
#include"CEnemy.h"
using ParticleManager = PoolManager<CParticle, 500>;

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

    //Imgui初期化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.5f; // フォントのスケールを倍に設定
    ImGui::StyleColorsDark(); // ダークモードに設定
    // Win32とDX11用の初期化（既存の機能からWindowとDeviceをもらう）
    ImGui_ImplWin32_Init(Application::GetWindow());
    ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetDeviceContext());

    RECT clientRect{};
    GetClientRect(Application::GetWindow(), &clientRect);
    Renderer::ResizeWindow(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    // カメラ初期化
    m_instance->m_Camera.Init();
    //最初のシーンを読み込む
    m_instance->m_scene = new Stage1Scene;//TODO:最初のシーンをタイトルシーンにする


}

// 更新
void Game::Update() {

    static int slowMoLevel = 1;
    static int frameCount = 0;
    frameCount++;

    // 現在のフレームでゲーム内時間を進めるかどうか
    bool doUpdate = (slowMoLevel == 1) || (frameCount % slowMoLevel == 0);

    //シーン更新
    Input::Update();
    //シーン更新

    if (doUpdate) {
        //シーン更新
        m_instance->m_scene->Update();
        // カメラ更新
        m_instance->m_Camera.Update();
    }

    RECT rect;
    GetClientRect(Application::GetWindow(), &rect);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // ImGuiフレーム開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //Imguiのウィンドウを作成
    ImGui::Begin("Player Debug");

    //TODO : ここにImguiの描画処理を追加する
    std::vector<CPlayer*> GUIPlayer = GetInstance()->GetObjects<CPlayer>();
    if (!GUIPlayer.empty() && GUIPlayer[0] != nullptr) {
        // 現在の座標を取得
        DirectX::SimpleMath::Vector3 pos = GUIPlayer[0]->GetPosition();



        ImGui::Separator(); // 区切り線を追加

        // テキストの場合
        // ImGui::Text("Player Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

        //--------------------------------------------------------------------------------
       // ImGuiのDragFloat3を使って座標を変更できるようにする
        //--------------------------------------------------------------------------------
        float posArray[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Player Pos", posArray, 0.5f)) {
            // UI上で数値が変更されたら、プレイヤーの座標にセットし直す
            GUIPlayer[0]->SetPosition(posArray[0], posArray[1], posArray[2]);
        }

        //-------------------------------------------------------------------------------------------
        //  ミサイルの最大数を変更するスライダー
        //-------------------------------------------------------------------------------------------
        int currentRocket = GUIPlayer[0]->getCurRocket();
        //1から10までの範囲でスライダーを作成※MaxRocketは4なので、4以上に設定しても4に制限される
        if (ImGui::SliderInt("Current Rocket", &currentRocket, 1, 10)) {
            // UI上で数値が変更されたら、プレイヤーのミサイル数にセットし直す
            GUIPlayer[0]->SetCurRocket(currentRocket);
        }

        ImGui::End();

        //-------------------------------------------------------------------------------------------

        // デバッグ情報の表示
        ImGui::Begin("Game Debug");
        ImGui::Text("FPS: %.1f", 1.0f / ImGui::GetIO().DeltaTime);

        ImGui::Text("Player HP: %d / %d", GUIPlayer[0]->GetHp(), GUIPlayer[0]->GetMaxHp());

        ImGui::Text("Missile Cooldown: %.2f / %.2f", GUIPlayer[0]->GetCurrentMissileTime(), GUIPlayer[0]->GetMissileTime());

        std::vector<ParticleManager*> pMgrs = GetInstance()->GetObjects<ParticleManager>();
        if (!pMgrs.empty()) {
            int active = pMgrs[0]->GetActiveCount();
            // 500は最大数。プログレスバーで視覚的に表示！
            ImGui::Text("Particles:");
            ImGui::ProgressBar((float)active / 500.0f, ImVec2(0.0f, 0.0f));
        }

        //---------------------------------------------------------------------------------------
        //  敵をスポーンさせるボタン
        //---------------------------------------------------------------------------------------
        if (ImGui::Button("madadekiteinai Spawn Enemy")) {
            int offsetX = 150;
            int offsetZ = 50;
            float randomX = (rand() % 40) - offsetX; // -20 ～ 20
            float randomZ = (rand() % 40) - offsetZ; // 10 ～ 50
            CEnemy* enemy = GetInstance()->AddObject<CEnemy>();
            enemy->SetPosition(randomX, -3.0f, randomZ);
            enemy->SetScale(1.0f, 1.0f, 1.0f);
        }
        //-------------------------------------------------------------------------------------------
        //  スローモーションスライダー
        //-------------------------------------------------------------------------------------------
        ImGui::SliderInt("Slow Motion (1=Norm, 5=Slow)", &slowMoLevel, 1, 5);

        Camera* cam = m_instance->GetCamera();
        DirectX::SimpleMath::Vector3 camOffset = cam->Getoffset();
        float offsetArr[3] = { camOffset.x, camOffset.y, camOffset.z };
        if (ImGui::DragFloat3("Camera Offset", offsetArr, 0.5f)) {
            cam->Setoffset(DirectX::SimpleMath::Vector3(offsetArr[0], offsetArr[1], offsetArr[2]));
        }

    }
    else {
        // プレイヤーがいない時の安全な表示
        ImGui::Text("Player is dead or not spawned.");
    }

    ImGui::End();

    if (doUpdate) {
        // テストオブジェクト更新
        for (auto& a : m_instance->m_objects) {
            a->Update();
        }

        for (auto& a : m_instance->m_UIs) {
            a->Update();
        }
    }

    // 削除フラグ（IsDead）が立っているオブジェクトを一括削除
    std::erase_if(m_instance->m_objects, [](const std::unique_ptr<Object>& obj) {
        if (obj->IsDead()) {
            obj->Uninit();
            return true;
        }
        return false;
    });

    std::erase_if(m_instance->m_UIs, [](const std::unique_ptr<Object>& obj) {
        if (obj->IsDead()) {
            obj->Uninit();
            return true;
        }
        return false;
    });

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

    for (auto& a : m_instance->m_UIs) {

        a->Draw(&m_instance->m_Camera);

    }

    Renderer::SetDepthEnable(true);

    ImGui::Render();//DrawEndの前に呼ぶ
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

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

    billboard::ReleaseTextures();//ビルボードのテクスチャ解放

    for (auto& a : m_instance->m_objects) {
        a->Uninit();
    }
    for (auto& a : m_instance->m_UIs) {
        a->Uninit();
    }

    // ImGui終了処理
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

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
