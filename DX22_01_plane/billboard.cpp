#include "billboard.h"
#include "Renderer.h"
#include "Game.h"

std::map<std::string, std::vector<Texture*>> billboard::s_textureGroups;

// 共有する画像配列の実体
std::vector<Texture*> billboard::m_sharedTextures;
int billboard::m_instanceCount = 0;

billboard::billboard() {
    m_maxFrames = 1;
    m_currentFrame = 0;
    m_animTimer = 0.0f;
    m_animSpeed = 0.1f;
    m_loop = false;
}

billboard::~billboard() {
    Uninit();
}





void billboard::Init() {
    m_instanceCount++;

    // 読み込んだ枚数をセット
    //m_maxFrames = m_sharedTextures.size();

    // 頂点とインデックスの初期化（Texture2Dクラスと同じ四角形の作成）
    m_Vertices.resize(4);
    m_Vertices[0].position = DirectX::SimpleMath::Vector3(-0.5f, 0.5f, 0);
    m_Vertices[1].position = DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0);
    m_Vertices[2].position = DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0);
    m_Vertices[3].position = DirectX::SimpleMath::Vector3(0.5f, -0.5f, 0);

    m_Vertices[0].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    m_Vertices[1].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    m_Vertices[2].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
    m_Vertices[3].color = DirectX::SimpleMath::Color(1, 1, 1, 1);

    m_Vertices[0].uv = DirectX::SimpleMath::Vector2(0, 0);
    m_Vertices[1].uv = DirectX::SimpleMath::Vector2(1, 0);
    m_Vertices[2].uv = DirectX::SimpleMath::Vector2(0, 1);
    m_Vertices[3].uv = DirectX::SimpleMath::Vector2(1, 1);

    m_VertexBuffer.Create(m_Vertices);

    m_Indices.resize(4);
    m_Indices[0] = 0; m_Indices[1] = 1; m_Indices[2] = 2; m_Indices[3] = 3;
    m_IndexBuffer.Create(m_Indices);

    m_Shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

    m_Material = std::make_unique<Material>();
    MATERIAL mtrl;
    mtrl.Diffuse = DirectX::SimpleMath::Color(1, 1, 1, 1);
    mtrl.TextureEnable = true;
    m_Material->Create(mtrl);
}

void billboard::SetAnim(float animSpeed, bool loop) {
    m_animSpeed = animSpeed;
    m_loop = loop;
    m_currentFrame = 0;
    m_animTimer = 0.0f;
}

void billboard::LoadTextures(const std::string& groupKey, const std::string& baseName, const std::string& ext, int count) {
    auto& textures = s_textureGroups[groupKey]; //[cite: 28]
    if (!textures.empty()) return; // このグループは既に読み込まれていたら何もしない[cite: 28]

    // もし count が 1 なら、連番をつけずにそのまま読み込む（スプライトシート用）
    if (count == 1) {
        Texture* tex = new Texture();
        std::string filename = baseName + ext;
        tex->Load(filename);
        textures.push_back(tex);
    }
    // 複数枚の連番画像を読み込む場合
    else {
        for (int i = 1; i <= count; ++i) {
            Texture* tex = new Texture();
            char filename[256];
            sprintf_s(filename, "%s%02d%s", baseName.c_str(), i, ext.c_str());
            tex->Load(filename);
            textures.push_back(tex);
        }
    }
}

void billboard::ReleaseTextures(const std::string& groupKey) {
    auto it = s_textureGroups.find(groupKey); 
    if (it == s_textureGroups.end()) return; 
    for (Texture* tex : it->second) delete tex; 
    s_textureGroups.erase(it); 
}

void billboard::Update() {
    // 簡易的なタイマー（60FPS想定なら 1.0f/60.0f を足す）
    m_animTimer += (1.0f / 60.0f);

    // 指定した時間を超えたら次の画像へ切り替え
    if (m_animTimer >= m_animSpeed) {
        m_animTimer = 0.0f;
        m_currentFrame++;

        if (m_SplitX > 1 || m_SplitY > 1) {
            // 1コマあたりの縦横のサイズ（割合）
            float u_step = 1.0f / m_SplitX;
            float v_step = 1.0f / m_SplitY;

            // 現在のコマ番号（0〜15）から、縦横の位置を割り出す
            int x_idx = m_currentFrame % m_SplitX;
            int y_idx = m_currentFrame / m_SplitX;

            // 切り抜く左上の座標
            float u = x_idx * u_step;
            float v = y_idx * v_step;

            // 4つの頂点のUV座標を書き換える
            // 頂点左上
            m_Vertices[0].uv = DirectX::SimpleMath::Vector2(u, v);                   // 左上
            m_Vertices[1].uv = DirectX::SimpleMath::Vector2(u + u_step, v);          // 右上
            m_Vertices[2].uv = DirectX::SimpleMath::Vector2(u, v + v_step);          // 左下
            m_Vertices[3].uv = DirectX::SimpleMath::Vector2(u + u_step, v + v_step); // 右下

            // 書き換えた頂点データをGPUに送る
            m_VertexBuffer.Modify(m_Vertices);

            // 最後の画像まで表示し終わったら
            if (m_currentFrame >= m_maxFrames) {
                if (m_loop) {
                    m_currentFrame = 0; // ループして最初に戻る
                }
                else {

                    m_currentFrame = m_maxFrames - 1; // 最後の画像で止める

                    //SetActive(false);
                }
            }
        }
    }
}

void billboard::Draw(Camera* cam) {
    auto it = s_textureGroups.find(m_groupKey); 
    if (m_maxFrames == 0 || it == s_textureGroups.end() || it->second.empty()) return; 

    // --- 1. ビルボード行列の計算 ---
    cam->SetCamera(0); // 3Dカメラをセット
    DirectX::SimpleMath::Matrix viewMtx = cam->GetViewMatrix();

    // 逆行列を作ると、カメラの真正面を向く
    DirectX::SimpleMath::Matrix billboardMtx = viewMtx.Invert();
    billboardMtx.Translation(DirectX::SimpleMath::Vector3::Zero); // 移動成分を消す

    // スケール・ビルボード回転・平行移動を合成
    DirectX::SimpleMath::Matrix s = DirectX::SimpleMath::Matrix::CreateScale(m_Scale);
    DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_Position);
    DirectX::SimpleMath::Matrix worldMtx = s * billboardMtx * t;

    Renderer::SetWorldMatrix(&worldMtx);

    // --- 2. UVはそのまま(1枚の画像だから) ---
    Renderer::SetUV(0, 0, 1, 1);

    // --- 3. 描画処理 ---
    Renderer::SetDepthEnable(true);

    m_Shader.SetGPU();
    m_VertexBuffer.SetGPU();
    m_IndexBuffer.SetGPU();


    // ここがパラパラ漫画のポイント！現在のフレームの画像をセットする
   // m_sharedTextures[0]->SetGPU();
    it->second[0]->SetGPU();

    m_Material->SetGPU();



    ID3D11DeviceContext* devicecontext = Renderer::GetDeviceContext();
    devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Renderer::SetBlendState(2);

    devicecontext->DrawIndexed((UINT)m_Indices.size(), 0, 0);

    Renderer::SetBlendState(1);
}

void billboard::Uninit() {
    m_instanceCount--;
    // ゲーム終了時までテクスチャは消さない（使い回す）設定にします
    
}
