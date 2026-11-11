/// @file GolfBall.cpp
/// @brief ゴルフ用ボールの移動、接地、衝突、描画。

#include "GolfBall.h"
#include "Collision.h"
#include "Game.h"
#include "Ground.h"
#include "Pole.h"

using namespace std;

using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void GolfBall::Init() {
    // メッシュ読み込み
    StaticMesh staticmesh;

    //3Dモデルデータ
    std::u8string modelFile = u8"assets/model/golfball/golf_ball.obj";

    //テクスチャディレクトリ
    std::string texDirectory = "assets/model/golfball/golf_ball.mtl";

    //Meshを読み込む
    std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    staticmesh.Load(tmpStr1, texDirectory);

    m_meshRenderer.Init(staticmesh);

    // シェーダオブジェクト生成
    m_shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    // サブセット情報取得
    m_subsets = staticmesh.GetSubsets();

    // テクスチャ情報取得
    m_textures = staticmesh.GetTextures();

    // マテリアル情報取得
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();

    // マテリアル数分ループ
    for (int i = 0; i < materials.size(); i++) {
        // マテリアルオブジェクト生成
        std::unique_ptr<Material> m = std::make_unique<Material>();

        // マテリアル情報をセット
        m->Create(materials[i]);

        // マテリアルオブジェクトを配列に追加
        m_materials.push_back(std::move(m));
    }

    //モデルによってスケールを調整
    m_scale.x = 1;
    m_scale.y = 1;
    m_scale.z = 1;

    m_velocity.x = 3.0f;
}

//=======================================
//更新処理
//=======================================
void GolfBall::Update() {
    if (m_state != 0) {
        return; //静止状態ならUpdate終了
    }
    Vector3 oldPos = m_position;

    if (m_velocity.LengthSquared() < 0.3f) { //この数字以下になったら強制ストップ
        //m_Velocity = Vector3(0.0f, 0.0f, 0.0f);
        m_stopCount++;
    } else {
        m_stopCount = 0;
        //減速度(１フレームでのどれくらい減速するか)
        float decelerationPower = 0.03f;
        Vector3 deceleration = -m_velocity; //速度の逆ベクトルを計算
        deceleration.Normalize(); //ベクトルを正規化
        m_acceleration = deceleration * decelerationPower;

        m_velocity += m_acceleration;
    }

    //10フレーム連続で動かなければ静止状態に遷移
    if (m_stopCount > 10) {
        m_velocity = Vector3(0.0f, 0.0f, 0.0f);
        m_state = 1;
    }
    //重力
    const float gravity = 0.1f;
    m_velocity.y -= gravity;
    //速度に重力を加算
    m_position += m_velocity;

    float radius = 1.5f; //ボールモデルの直系

    //Groundの頂点データを取得
    vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    vector<VERTEX_3D> vertices;
    for (auto& g : grounds) { //Groundオブジェクトの数ループ
        vector<VERTEX_3D> vecs = g->GetVertices();
        for (auto& v : vecs) { //頂点の数ループ
            vertices.emplace_back(v);
        }
    }

    float moveDistance = 9999; //移動距離
    Vector3 contactPoint; //接触点
    Vector3 normal;

    //地面との当たり判定
    for (int i = 0; i < vertices.size(); i += 3) {
        Collision::Polygon collisionPolygon = {
            vertices[i + 0].position,
            vertices[i + 1].position,
            vertices[i + 2].position,
        };
        Vector3 cp;
        Collision::Segment collisionsegment = {oldPos, m_position};
        Collision::Sphere collisinSphere = {m_position, radius};

        if (Collision::CheckHit(collisionsegment, collisionPolygon, cp)) {
            float md = 0;
            Vector3 np = Collision::moveSphere(collisionsegment, radius, collisionPolygon, cp, md);

            if (moveDistance > md) {
                moveDistance = md;
                m_position = np;
                contactPoint = cp;
                normal = Collision::GetNormal(collisionPolygon);
            }
        } else if (Collision::CheckHit(collisinSphere, collisionPolygon, cp)) {
            Vector3 np = Collision::moveSphere(collisinSphere, collisionPolygon, cp);
            float md = (np - oldPos).Length();
            if (moveDistance > md) {
                moveDistance = md;
                m_position = np;
                contactPoint = cp;
                normal = Collision::GetNormal(collisionPolygon);
            }
        }
    }
    if (moveDistance != 9999) {
        float velocitynormal = Collision::Dot(m_velocity, normal);
        Vector3 v1 = velocitynormal * normal;
        Vector3 v2 = m_velocity - v1;

        const float restitution = 0.02f;
        const float friction = 0.90f;
        Vector3 reflectedVelocity = v2 * friction - v1 * restitution;

        m_velocity = reflectedVelocity;
    }
    if (m_position.y < -1000) { //もしも下に落ちた時にリスポーンする用
        m_position = Vector3(0.0f, 50.0f, 0.0f); //リスポーン地点
        m_velocity = Vector3(0.0f, 0.0f, 0.0f); //速度を０に
    }

    //Poleの位置を取得
    vector<Pole*> pole = Game::GetInstance()->GetObjects<Pole>();
    if (pole.size() > 0) {
        Vector3 polepos = pole[0]->GetPosition();
        Collision::Sphere balcollision = {m_position, radius}; //ゴルフボールの当たり判定
        Collision::Sphere polecollision = {polepos, 1.5f}; //ポールの当たり判定
        if (Collision::CheckHit(balcollision, polecollision)) {
            m_state = 2; //カップイン
        }
    }
}

//=======================================
//描画処理
//=======================================
void GolfBall::Draw(Camera* cam) {
    //カメラを選択する
    cam->SetCamera();

    cam->SetTarget(m_position);

    // SRT情報作成
    Matrix r = Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z);
    Matrix t = Matrix::CreateTranslation(m_position.x, m_position.y, m_position.z);
    Matrix s = Matrix::CreateScale(m_scale.x, m_scale.y, m_scale.z);

    Matrix worldmtx;
    worldmtx = s * r * t;
    Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

    m_shader.SetGPU();

    // インデックスバッファ・頂点バッファをセット
    m_meshRenderer.BeforeDraw();

    //マテリアル数分ループ
    for (int i = 0; i < m_subsets.size(); i++) {
        // マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
        m_materials[m_subsets[i].MaterialIdx]->SetGPU();

        if (m_materials[m_subsets[i].MaterialIdx]->isTextureEnable()) {
            m_textures[m_subsets[i].MaterialIdx]->SetGPU();
        }

        m_meshRenderer.DrawSubset(m_subsets[i].IndexNum, // 描画するインデックス数
                                  m_subsets[i].IndexBase, // 最初のインデックスバッファの位置
                                  m_subsets[i].VertexBase); // 頂点バッファの最初から使用
    }
}

//=======================================
//終了処理
//=======================================
void GolfBall::Uninit() {}

void GolfBall::SetColor(DirectX::SimpleMath::Color col) {
    // 持っているすべてのマテリアルの色を変える
    for (auto& mat : m_materials) {
        // さっきMaterial.hで作った関数を呼ぶ
        mat->SetColor(col.x, col.y, col.z, col.w);
    }
}
