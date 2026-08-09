#include "CParticle.h"
#include "Game.h"
#include"Renderer.h"

// static変数の実体を定義
TestCube* CParticle::m_body = nullptr;
int CParticle::m_instanceCount = 0;

//Spark用のstatic変数の実体を定義
Texture* CParticle::m_sparkTexture = nullptr;
Shader* CParticle::m_sparkShader = nullptr;
VertexBuffer<VERTEX_3D>* CParticle::m_sparkVB = nullptr;
IndexBuffer* CParticle::m_sparkIB = nullptr;
Material* CParticle::m_sparkMaterial = nullptr;

CParticle::CParticle() {
    if (m_type == Test) {
        m_life = 60.0f;
    }

    m_color = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f );

}

CParticle::~CParticle() {
    Uninit();
}

void CParticle::Init() {
    // 「一番最初の1個目」が作られた時だけ、モデルを読み込む
    Preload();
    m_instanceCount++; // 生成されたらカウントを増やす
}

void CParticle::Update() {

    if (m_type == Test) {
        // 速度の分だけ移動
        m_Position += m_velocity;
        m_velocity.y -= 0.02f; // 重力

        // クルクル回転させる
        m_Rotation.x += 0.1f;
        m_Rotation.y += 0.1f;

    }
    else if (m_type == Spark) {
        m_Scale.x *= 0.95f; // 徐々に小さくなる
        m_Scale.y *= 0.95f;
        m_Scale.z *= 0.95f;
    }


    // 寿命を減らして、0になったら自分を消す
    m_life -= 1.0f;
    if (m_life <= 0.0f) {
        Destroy();
    }
}

void CParticle::Draw(Camera* cam) {
    if (m_type == Test && m_body) {
        m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z);
        m_body->SetRotation(m_Rotation);
        m_body->SetScale(m_Scale.x, m_Scale.y, m_Scale.z); // ★ スケールを適用！
        m_body->Draw(cam);
    }
    //  Sparkの場合は、板（ビルボード）として画像を描く
    else if (m_type == Spark && m_sparkTexture) {
        Renderer::SetBlendState(2); // 加算合成（光るモード）

        cam->SetCamera(0); // 3Dカメラを使用
        DirectX::SimpleMath::Matrix viewMtx = cam->GetViewMatrix();

        // カメラの逆行列を使って、板を常にカメラの真正面に
        DirectX::SimpleMath::Matrix billboardMtx = viewMtx.Invert();
        billboardMtx.Translation(DirectX::SimpleMath::Vector3::Zero);

        DirectX::SimpleMath::Matrix s = DirectX::SimpleMath::Matrix::CreateScale(m_Scale);
        DirectX::SimpleMath::Matrix t = DirectX::SimpleMath::Matrix::CreateTranslation(m_Position);
        DirectX::SimpleMath::Matrix worldMtx = s * billboardMtx * t;

        Renderer::SetWorldMatrix(&worldMtx);
        Renderer::SetUV(0, 0, 1, 1);

        m_sparkShader->SetGPU();
        m_sparkVB->SetGPU();
        m_sparkIB->SetGPU();
        m_sparkTexture->SetGPU();
        m_sparkMaterial->SetGPU();

        MATERIAL mtrl;
        mtrl.Diffuse = m_color; // パーティクルの色をマテリアルに反映
        mtrl.Ambient = DirectX::SimpleMath::Color(1.0f, 1.0f, 1.0f, 1.0f);
        mtrl.TextureEnable = true;
        Renderer::SetMaterial(mtrl);

        ID3D11DeviceContext* dc = Renderer::GetDeviceContext();
        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        dc->DrawIndexed(4, 0, 0);

        Renderer::SetBlendState(1); // 通常の半透明に戻す
    }
}

void CParticle::Uninit() {

}

void CParticle::Preload() {
    if (m_body == nullptr) {
        m_body = new TestCube;
        m_body->Init();
        m_body->SetScale(0.5f, 0.5f, 0.5f);
    }

    if (m_sparkTexture == nullptr) {
        m_sparkTexture = new Texture();
        m_sparkTexture->Load("assets/texture/spark.png"); // ★ ここで画像をロード！

        m_sparkShader = new Shader();
        m_sparkShader->Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");

        // 板の頂点（4つの角）
        std::vector<VERTEX_3D> vertices(4);
        vertices[0].position = DirectX::SimpleMath::Vector3(-0.5f, 0.5f, 0);
        vertices[1].position = DirectX::SimpleMath::Vector3(0.5f, 0.5f, 0);
        vertices[2].position = DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0);
        vertices[3].position = DirectX::SimpleMath::Vector3(0.5f, -0.5f, 0);
        for (int i = 0; i < 4; i++) vertices[i].color = DirectX::SimpleMath::Color(1, 1, 1, 1);
        vertices[0].uv = DirectX::SimpleMath::Vector2(0, 0);
        vertices[1].uv = DirectX::SimpleMath::Vector2(1, 0);
        vertices[2].uv = DirectX::SimpleMath::Vector2(0, 1);
        vertices[3].uv = DirectX::SimpleMath::Vector2(1, 1);

        m_sparkVB = new VertexBuffer<VERTEX_3D>();
        m_sparkVB->Create(vertices);

        std::vector<unsigned int> indices = { 0, 1, 2, 3 };
        m_sparkIB = new IndexBuffer();
        m_sparkIB->Create(indices);

        m_sparkMaterial = new Material();
        MATERIAL mtrl;
        mtrl.Diffuse = DirectX::SimpleMath::Color(1, 1, 1, 1);
        mtrl.TextureEnable = true;
        m_sparkMaterial->Create(mtrl);
    }

}
