#include"Application.h"//GetWindow()を使うために必要
#include "Game.h"
#include "Renderer.h"
#include"input.h"
#include"billboard.h"

#include"imgui.h"
#include"imgui_impl_dx11.h"
#include"imgui_impl_win32.h"
#include "CPlayer.h"
#include "CAutoturret.h"
#include "CBossBattle.h"
#include "PoolManager.h"
#include <filesystem>

#ifdef _DEBUG
#include"CPlayerUI.h"
#include"CParticle.h"
#include"CEnemy.h"
#include"CBullet.h"
#include"CMissile.h"
#include "CPresentBox.h"
#endif
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

    // 成長UIはゲーム本体の機能なので、ReleaseでもImGuiを初期化する。
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.0f;
    // UTF-8の武器名を表示できるようWindowsの日本語フォントを読み込む。
    char windowsDirectory[MAX_PATH]{};
    GetWindowsDirectoryA(windowsDirectory, MAX_PATH);
    const std::string fontPath = std::string(windowsDirectory) + "/Fonts/meiryo.ttc";
    if (std::filesystem::exists(fontPath)) {
        // 標準の日本語セットにない「填」なども、スキル説明から必要な文字を追加する。
        // フォント構築は後のフレームなので、範囲の配列をstaticで保持する。
        static ImVector<ImWchar> skillGlyphRanges;
        ImFontGlyphRangesBuilder glyphs;
        glyphs.AddRanges(io.Fonts->GetGlyphRangesJapanese());
        for (int i = 0; i < static_cast<int>(UpgradeId::Count); ++i) {
            const auto id = static_cast<UpgradeId>(i);
            const auto& definition = GetUpgradeDefinition(id);
            glyphs.AddText(reinterpret_cast<const char*>(definition.name));
            glyphs.AddText(reinterpret_cast<const char*>(definition.description));
            glyphs.AddText(GetUpgradeEffectText(id, definition.maxRank).c_str());
        }
        glyphs.BuildRanges(&skillGlyphRanges);
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, nullptr, skillGlyphRanges.Data);
    }
    ImGui::StyleColorsDark(); // ダークモードに設定
    // Win32とDX11用の初期化（既存の機能からWindowとDeviceをもらう）
    ImGui_ImplWin32_Init(Application::GetWindow());
    ImGui_ImplDX11_Init(Renderer::GetDevice(), Renderer::GetDeviceContext());

    RECT clientRect{};
    GetClientRect(Application::GetWindow(), &clientRect);
    Renderer::ResizeWindow(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    // カメラ初期化
    m_instance->m_Camera.Init();

#ifdef _DEBUG
    m_instance->m_DebugSphere.Init();
#endif

    //最初のシーンを読み込む
    m_instance->m_scene = new Stage1Scene;//TODO:最初のシーンをタイトルシーンにする

    
}

// 更新
void Game::Update(double elapsedSeconds) {

    static int slowMoLevel = 1;

    auto players = m_instance->GetObjects<CPlayer>();
    CPlayer* player = players.empty() ? nullptr : players[0];
    const bool paused = player && player->IsGameplayPaused();

    // スロー再生はフレームを間引かず、ゲーム内で進む秒数を小さくする。
    // 一時停止中も毎フレーム計測し、再開時に停止時間を加算しない。
    GameTime::BeginFrame(elapsedSeconds, 1.0f / slowMoLevel, paused);
    Input::Update();

    while (GameTime::NextStep()) {
        //シーン更新
        m_instance->m_scene->Update();
        // シーンが切り替わった場合は、破棄済みのプレイヤーを参照しない。
        players = m_instance->GetObjects<CPlayer>();
        player = players.empty() ? nullptr : players[0];
        // カメラ更新
        m_instance->m_Camera.Update();

        // 表示時間と敵の出現段階も、武器・移動と同じ経過秒数で進める。
        m_instance->m_gameTime += GameTime::DeltaSeconds();

        // テストオブジェクト更新
        for (auto& a : m_instance->m_objects) {
            if (!a->IsDead()) a->Update();
            // 更新途中でレベルアップした場合も、それ以降の敵から被弾させない。
            if (player && player->IsGameplayPaused()) break;
        }

        for (auto& a : m_instance->m_UIs) {
            if (!a->IsDead() && !(player && player->IsGameplayPaused())) a->Update();
        }

        // 低FPSで複数回更新しても、押した瞬間の入力は1回だけ扱う。
        Input::ConsumeTriggers();
        FlushObjects();
        if (player && player->IsGameplayPaused()) break;
    }

    FlushObjects();

    RECT rect;
    GetClientRect(Application::GetWindow(), &rect);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // ImGuiフレーム開始
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

#ifdef _DEBUG
    // 数字キー1～3は報酬選択に使用するため、以前のHPデバッグショートカットは廃止。
    std::vector<CPlayer*> GUIPlayer = GetInstance()->GetObjects<CPlayer>();
    if (!GUIPlayer.empty() && !GUIPlayer[0]->IsGameplayPaused()) {
    //Imguiのウィンドウを作成
    ImGui::Begin("Player Debug");

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
        //1から20までの範囲でスライダーを作成※MaxRocketは20なので、20以上に設定しても20に制限される
        if (ImGui::SliderInt("Current Rocket", &currentRocket, 1, 20)) {
            // UI上で数値が変更されたら、プレイヤーのミサイル数にセットし直す
            GUIPlayer[0]->SetCurRocket(currentRocket);
        }

        ImGui::End();

        //-------------------------------------------------------------------------------------------

        // デバッグ情報の表示
        ImGui::Begin("Game Debug");
        ImGui::Text("FPS: %.1f", 1.0f / ImGui::GetIO().DeltaTime);

        ImGui::Text("Player HP: %d / %d", GUIPlayer[0]->GetHp(), GUIPlayer[0]->GetMaxHp());

        ImGui::Text("Missile Cooldown (sec): %.2f / %.2f", GUIPlayer[0]->GetCurrentMissileTime(), GUIPlayer[0]->GetMissileTime());

        ImGui::Text("GameTime:%.2f", GetInstance()->m_gameTime);
        // 成長UIと実際の箱の取得を、待ち時間なしで確認するためのデバッグ操作。
        if (ImGui::Button("Test kill (+20 EXP)")) GUIPlayer[0]->OnEnemyDefeated(20);
        if (ImGui::Button("Drop present here")) {
            auto box = GetInstance()->AddObject<CPresentBox>();
            box->SetPosition(GUIPlayer[0]->GetPosition());
        }
        if (ImGui::Button("Restore HP")) GUIPlayer[0]->SetHP(GUIPlayer[0]->GetMaxHp());

        // 武器の抽選を待たずに、砲台の攻撃方向とレーザー表示を確認するための操作。
        if (ImGui::Button("Get / upgrade companion"))
        {
            // デバッグでも通常の取得処理を通し、別の砲台を増やさない。
            GUIPlayer[0]->DebugGiveUpgrade(UpgradeId::Turret);
        }
        
        //-------------------------------------------------------------------------------------------
        //Particleのオブジェクトプール表示
        //-------------------------------------------------------------------------------------------
        std::vector<ParticleManager*> pMgrs = GetInstance()->GetObjects<ParticleManager>();
        if (!pMgrs.empty()) {
            int active = pMgrs[0]->GetActiveCount();
            // 500は最大数。プログレスバーで視覚的に表示！
            ImGui::Text("Particles:");
            float MAX = ParticleManager::MAXSIZE;
            ImGui::ProgressBar((float)active / MAX, ImVec2(0.0f, 0.0f));
        }

        
        std::vector<BulletManager*> bMgrs = GetInstance()->GetObjects<BulletManager>();
        if (!bMgrs.empty()) {
            int active = bMgrs[0]->GetActiveCount();
            ImGui::Text("Bullets:");
            float maxCount = (float)BulletManager::MAXSIZE;
            // わかりやすいように色を変えることもできます（例：黄色）
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
            ImGui::ProgressBar((float)active / maxCount, ImVec2(0.0f, 0.0f));
            ImGui::PopStyleColor();
        }

        std::vector<MissileManager*> mMgrs = GetInstance()->GetObjects<MissileManager>();
        if (!mMgrs.empty()) {
            int active = mMgrs[0]->GetActiveCount();
            ImGui::Text("Missiles:");
            float maxCount = (float)MissileManager::MAXSIZE;
            // ミサイルは赤色にしてみる
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::ProgressBar((float)active / maxCount, ImVec2(0.0f, 0.0f));
            ImGui::PopStyleColor();
        }

        //---------------------------------------------------------------------------------------
        //  敵をスポーンさせるボタン
        //---------------------------------------------------------------------------------------
        bool bossBattleActive = false;
        for (auto battle : GetInstance()->GetObjects<CBossBattle>())
        {
            bossBattleActive = bossBattleActive || battle->IsBattleActive();
        }
        ImGui::BeginDisabled(bossBattleActive); // 一騎打ち中はDebugからも雑魚を追加しない。
        if (ImGui::Button("madadekiteinai Spawn Enemy")) {
            int offsetX = 150;
            int offsetZ = 50;
            float randomX = (rand() % 40) - offsetX; // -20 ～ 20
            float randomZ = (rand() % 40) - offsetZ; // 10 ～ 50
            CEnemy* enemy = GetInstance()->AddObject<CEnemy>();
            enemy->SetPosition(randomX, -3.0f, randomZ);
            enemy->SetScale(1.0f, 1.0f, 1.0f);
        }
        ImGui::EndDisabled();
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
        //-------------------------------------------------------------------------------------------
        // 当たり判定のサイズをリアルタイムに変更するスライダー
         //-------------------------------------------------------------------------------------------
        ImGui::Separator();
        ImGui::Text("Collision Radius Settings");
        if (ImGui::Button(m_instance->m_ShowCollision ? "Collision (ON)" : "Collision (OFF)")) {
            m_instance->m_ShowCollision = !m_instance->m_ShowCollision;
        }
       

        // 弾の半径変更
        static float debugBulletRadius = 2.0f;
        if (ImGui::SliderFloat("Bullet Radius", &debugBulletRadius, 1.0f, 50.0f)) {
            std::vector<BulletManager*> bMgrs = GetInstance()->GetObjects<BulletManager>();
            if (!bMgrs.empty()) {
                // プール内のすべての弾のサイズを書き換える
                for (auto b : bMgrs[0]->GetPool()) {
                    b->SetColRadius(debugBulletRadius);
                }
            }
        }

        // ミサイルの半径変更
        static float debugMissileRadius = 20.0f;
        if (ImGui::SliderFloat("Missile Radius", &debugMissileRadius, 1.0f, 100.0f)) {
            std::vector<MissileManager*> mMgrs = GetInstance()->GetObjects<MissileManager>();
            if (!mMgrs.empty()) {
                // プール内のすべてのミサイルのサイズを書き換える
                for (auto m : mMgrs[0]->GetPool()) {
                    m->SetColRadius(debugMissileRadius);
                }
            }
        }

    }
    else {
        // プレイヤーがいない時の安全な表示
        ImGui::Text("Player is dead or not spawned.");
    }

    ImGui::End();

    }
#endif // _DEBUG
    // 停止中にも操作できるデバッグ画面。通常の報酬画面とは別に描画する。
#ifdef _DEBUG
    for (CPlayer* survivor : GetInstance()->GetObjects<CPlayer>()) survivor->DrawSurvivorDebugUI();
#endif
    // 同じUIをDebug / Releaseの両方で使用する。
    for (CPlayer* survivor : GetInstance()->GetObjects<CPlayer>()) survivor->DrawSurvivorUI();
    for (auto battle : GetInstance()->GetObjects<CBossBattle>()) battle->DrawUI();
   
}

void Game::FlushObjects()
{
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
        if (!a->IsDead()) a->Draw(&m_instance->m_Camera);
    }
    if (m_instance->m_scene) {
        m_instance->m_scene->Draw(&m_instance->m_Camera);
    }

    // 不透明な地形・敵を描いた後で、砲台のレーザーを重ねる。
    // 後から出現した敵に光線が不自然に上書きされることを防ぐ。
    for (auto manager : m_instance->GetObjects<AutoTurretManager>())
    {
        for (auto turret : manager->GetPool())
        {
            turret->DrawLaser(&m_instance->m_Camera);
        }
    }

    // 透明なバリアは地形と敵を描き終えてから重ねる。
    for (auto battle : m_instance->GetObjects<CBossBattle>()) battle->DrawBarrier(&m_instance->m_Camera);

    Renderer::SetDepthEnable(false);

    for (auto& a : m_instance->m_UIs) {

        a->Draw(&m_instance->m_Camera);

    }

    Renderer::SetDepthEnable(true);
#ifdef _DEBUG
    if (m_instance->m_ShowCollision) {
        Renderer::SetWireFrame(true); // 線だけモードに変更
        Renderer::SetBlendState(2);   // 加算合成（光らせる）

        auto drawSphere = [](Object* obj) {
            Collision::Sphere s = obj->GetCollisionSphere();
            if (s.radius > 0.0f) {
                m_instance->m_DebugSphere.SetPosition(s.center);
                float scale = s.radius / 10.0f;
                m_instance->m_DebugSphere.SetScale(scale, scale, scale);

                m_instance->m_DebugSphere.Draw(&m_instance->m_Camera);
            }
            };

        // 1. 通常のオブジェクト（Player, Enemy）を描画
        for (auto& a : m_instance->m_objects) {
            drawSphere(a.get());

            // 2. プールマネージャーなら、中身の弾やミサイルも描画
            if (auto pBullet = dynamic_cast<BulletManager*>(a.get())) {
                for (auto b : pBullet->GetPool()) { if (b->GetActive()) drawSphere(b); }
            }
            if (auto pMissile = dynamic_cast<MissileManager*>(a.get())) {
                for (auto m : pMissile->GetPool()) { if (m->GetActive()) drawSphere(m); }
            }
        }

        Renderer::SetBlendState(1);    // 通常の半透明に戻す
        Renderer::SetWireFrame(false); // 塗りつぶしモードに戻す
    }
#endif // _DEBUG
    ImGui::Render();
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

    billboard::ReleaseTextures("smoke");//ビルボードのテクスチャ解放

    /*
    for (auto& a : m_instance->m_objects) {
        a->Uninit();
    }
    for (auto& a : m_instance->m_UIs) {
        a->Uninit();
    }
    */
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
    // 子UIを参照するオブジェクトがあるため、先に全Uninitを済ませてからメモリを解放する。
    for (auto& o : m_instance->m_objects) {
        o->Uninit();
    }
    for (auto& o : m_instance->m_UIs) {
        o->Uninit();
    }
    for (auto& o : m_instance->m_addObjects) o->Uninit();
    for (auto& o : m_instance->m_addUIs) o->Uninit();
    m_instance->m_objects.clear();
    m_instance->m_UIs.clear();
    m_instance->m_addObjects.clear();
    m_instance->m_addUIs.clear();

}
