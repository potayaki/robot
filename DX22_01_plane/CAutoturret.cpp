/// @file CAutoturret.cpp
/// @brief プレイヤーに追従する目型砲台の照準・攻撃・レーザー表示。

#include "CAutoturret.h"
#include "EnemyTarget.h"
#include <vector>
#include "Game.h"
#include "Renderer.h"
#include "CPlayer.h"

using namespace DirectX::SimpleMath;

CAutoturret::CAutoturret() = default;

CAutoturret::~CAutoturret() {
    Uninit();
}

void CAutoturret::Init() {
    m_eye.Init(); // 弾のモデルを使わず、専用の眼球を生成する。
    InitLaser();
}

void CAutoturret::Update() {
    const float deltaSeconds = GameTime::DeltaSeconds();
    // 残光が消えるフレームでは、消えた後に残っている秒数だけ回転に使う。
    const float turnSeconds = (std::max)(0.0f, deltaSeconds - m_laserTime);
    // 相棒には寿命を設けない。プレイヤーがいない間や死亡中は非表示にする。
    GameTime::CountDown(m_laserTime, deltaSeconds);
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        m_laserTime = 0;
        m_active = false;

        return;
    }

    const Vector3 oldPosition = m_position;
    m_position =
        EyeTurretShape::FollowPosition(m_position, players[0]->GetTurretFollowPosition(), deltaSeconds);
    if (m_laserTime > 0.0f) {
        // 短い残光も目と一緒に移動させ、追従中に瞳と光線が離れないようにする。
        // 残光は表示のみで、追加のダメージ判定は行わない。
        const Vector3 movement = m_position - oldPosition;
        m_laserStart += movement;
        m_laserEnd += movement;
    }

    // 自動砲台もドローンを攻撃対象として認識する。
    std::vector<EnemyTarget*> enemies = Game::GetInstance()->GetObjects<EnemyTarget>();
    EnemyTarget* minEnemy = nullptr;
    float minDistance = 9999.0f; // 十分大きな値で初期化

    for (EnemyTarget* enemy : enemies) {
        if (enemy == nullptr || enemy->IsDead() || !enemy->GetActive()) {
            continue;
        }
        float distance = (enemy->GetCollisionSphere().center - GetEyeCenter()).Length();

        if (distance < minDistance) {
            minDistance = distance;
            minEnemy = enemy;
        }
    }

    GameTime::AdvanceCooldown(m_attackCooldown, deltaSeconds);

    // ビームが消えている間は次の標的へ目を向ける。
    // 発射後の残光中は視線を固定し、瞳だけが別の敵へ向くことを防ぐ。
    const Vector3 eyeCenter = GetEyeCenter();
    bool aimedAtTarget = false;
    if (minEnemy != nullptr && minDistance < m_radius && m_laserTime <= 0.0f) {
        const Vector3 targetDirection = minEnemy->GetCollisionSphere().center - eyeCenter;
        aimedAtTarget = EyeTurretShape::TurnTowards(m_eyeRotation, targetDirection, turnSeconds);
    }

    // 照準が合うまで待つ。発射時だけ敵の向きへ瞬間移動させない。
    if (aimedAtTarget && m_attackCooldown <= 0.0f) {
        // ダメージを与える前に実際の攻撃先を記録する。
        // 表示中は敵を追いかけず、今回撃った方向に短い残光を残す。
        m_laserEnd = minEnemy->GetCollisionSphere().center;
        const Vector3 lookDirection = EyeTurretShape::LookDirection(m_eyeRotation);
        m_laserStart = EyeTurretShape::BeamStart(eyeCenter, m_laserEnd, lookDirection);
        m_laserTime = LaserDurationSeconds;

        minEnemy->OnHit(m_damage);
        m_attackCooldown += m_attackInterval;
    }
}

void CAutoturret::ConfigureUpgrade(int rank) {
    // 既存の1体を強化し、毎レベル攻撃力・射程・連射速度を上げる。
    const auto stats = GetTurretStats(rank);
    m_damage = stats.damage;
    m_radius = stats.range;
    m_attackInterval = stats.attackInterval;
    m_attackCooldown = (std::min)(m_attackCooldown, m_attackInterval);
}

void CAutoturret::Draw(Camera* cam) {
    m_eye.Draw(cam, GetEyeCenter(), m_eyeRotation);
    // レーザーはGame::Drawの後半で描く。ここでは不透明な砲台本体だけを描画する。
}

void CAutoturret::InitLaser() {
    m_laserReady = false;
    m_laserTime = 0;

    // 既存の白い画像と陰影なしシェーダを使用し、照明に左右されない光線にする。
    if (!m_laserTexture.Load("assets/texture/white.png")) {
        return;
    }

    m_laserShader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
    m_laserVertices.resize(12); // 1枚6頂点 × 外側の光と内側の芯。
    m_laserBuffer.Create(m_laserVertices);

    // 細い板を裏から見ても消えないよう、この描画だけ裏面カリングを無効にする。
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;

    const HRESULT result =
        Renderer::GetDevice()->CreateRasterizerState(&rasterizer, m_laserRasterizer.ReleaseAndGetAddressOf());

    m_laserReady = SUCCEEDED(result);
}

void CAutoturret::DrawLaser(Camera* cam) {
    if (!m_active || !m_laserReady || m_laserTime <= 0 || cam == nullptr) {
        return;
    }

    const Vector3 direction = m_laserEnd - m_laserStart;
    if (direction.LengthSquared() < 0.000001f) {
        return;
    }

    // 始点・終点を動かさず、板の幅方向だけをカメラへ向ける。
    // 地上のスライムにも空中のドローンにも、同じ方法で線を引ける。
    cam->SetCamera();
    const Matrix inverseView = cam->GetViewMatrix().Invert();
    const Vector3 center = (m_laserStart + m_laserEnd) * 0.5f;
    const Vector3 toCamera = inverseView.Translation() - center;
    Vector3 side = direction.Cross(toCamera);

    // 光線を真正面から見る場合は外積が0になるので、別の軸で幅方向を作る。
    if (side.LengthSquared() < 0.000001f) {
        side = direction.Cross(Vector3(0.0f, 1.0f, 0.0f));
    }
    if (side.LengthSquared() < 0.000001f) {
        side = direction.Cross(Vector3(1.0f, 0.0f, 0.0f));
    }
    side.Normalize();

    // 外側は淡い水色、芯は白に近い水色。残り時間に合わせて透明度を下げる。
    const float fade = m_laserTime / LaserDurationSeconds;
    const auto setStrip = [&](int firstVertex, float width, const Color& color) {
        const Vector3 offset = side * (width * 0.5f);
        const Vector3 corners[4] = {m_laserStart - offset, m_laserStart + offset, m_laserEnd - offset,
                                    m_laserEnd + offset};
        const int indices[6] = {0, 1, 2, 2, 1, 3};

        for (int i = 0; i < 6; ++i) {
            VERTEX_3D& vertex = m_laserVertices[firstVertex + i];
            vertex.position = corners[indices[i]];
            vertex.color = color;
            vertex.uv = Vector2(0.5f, 0.5f); // 白い画像の中央だけを使用する。
        }
    };

    setStrip(0, 4.0f, Color(0.1f, 0.6f, 1.0f, 0.3f * fade));
    setStrip(6, 1.0f, Color(0.7f, 0.95f, 1.0f, 0.95f * fade));
    m_laserBuffer.Modify(m_laserVertices);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // 描画状態を保存し、このレーザーの設定を後続のUIや他のオブジェクトに残さない。
    Microsoft::WRL::ComPtr<ID3D11BlendState> previousBlend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> previousDepth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> previousRasterizer;
    float previousBlendFactor[4]{};
    UINT previousSampleMask = 0;
    UINT previousStencilReference = 0;
    D3D11_PRIMITIVE_TOPOLOGY previousTopology;

    context->OMGetBlendState(previousBlend.GetAddressOf(), previousBlendFactor, &previousSampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &previousStencilReference);
    context->RSGetState(previousRasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&previousTopology);

    Renderer::SetBlendState(BS_ADDITIVE);
    // このプロジェクトのfalse設定は、深度テストを残して深度の書き込みだけを止める。
    // 地形や敵に隠れる部分は隠しつつ、透明な光で他の描画を遮らないようにする。
    Renderer::SetDepthEnable(false);
    context->RSSetState(m_laserRasterizer.Get());

    Matrix world = Matrix::Identity; // 頂点にワールド座標を直接設定している。
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    m_laserShader.SetGPU();
    m_laserTexture.SetGPU();
    m_laserBuffer.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(m_laserVertices.size()), 0);

    context->OMSetBlendState(previousBlend.Get(), previousBlendFactor, previousSampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), previousStencilReference);
    context->RSSetState(previousRasterizer.Get());
    context->IASetPrimitiveTopology(previousTopology);
}

void CAutoturret::Uninit() {
    // プールの終了処理とデストラクタの両方から呼ばれても安全に解放する。
    m_laserTime = 0;
    m_laserReady = false;
    m_laserRasterizer.Reset();
    m_laserBuffer = VertexBuffer<VERTEX_3D>{};
    m_laserTexture = Texture{};
    m_laserShader = Shader{};
    m_laserVertices.clear();

    m_eye.Uninit();
}
