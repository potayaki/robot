#include "PlayerRobotModel.h"
#include <iostream>

using namespace DirectX::SimpleMath;

namespace
{
    // 素材の高さ約789・脚の幅約1086を、高さ約20・幅約27へ縮小する。
    constexpr float RobotScale = 0.025f;
    constexpr float PlayerCollisionRadius = 10.0f;

    Matrix ToDirectX(const aiMatrix4x4& m)
    {
        // Assimpは列ベクトル、SimpleMathは行ベクトルなので転置して渡す。
        return Matrix(m.a1, m.b1, m.c1, m.d1,
                      m.a2, m.b2, m.c2, m.d2,
                      m.a3, m.b3, m.c3, m.d3,
                      m.a4, m.b4, m.c4, m.d4);
    }

    aiMatrix4x4 ToAssimp(const Matrix& m)
    {
        return aiMatrix4x4(m._11, m._21, m._31, m._41,
                           m._12, m._22, m._32, m._42,
                           m._13, m._23, m._33, m._43,
                           m._14, m._24, m._34, m._44);
    }
}

PlayerRobotModel::SkinnedVertex PlayerRobotModel::MakeVertex(const RobotAnimationRig::Vertex& source)
{
    SkinnedVertex vertex{};
    vertex.vertex.position = Vector3(source.position.x, source.position.y, source.position.z);
    vertex.vertex.normal = Vector3(source.normal.x, source.normal.y, source.normal.z);
    vertex.vertex.uv = Vector2(source.uv.x, source.uv.y);
    vertex.vertex.color = Color(1, 1, 1, 1);
    vertex.bone = source.node;
    return vertex;
}

void PlayerRobotModel::Init()
{
    Uninit();
    std::string error;
    if (!m_rig.Load("assets/model/Player", error))
    {
        std::cerr << "Player robot: " << error << '\n';
        return;
    }
    if (m_rig.GetMatrices().size() > MaxBones) return;
    m_ikReady = m_spiderIK.Init(m_rig);
    if (!m_ikReady) std::cerr << "Player six-leg IK could not find the expected joints.\n";
    // FBX内の画像参照は配布フォルダと一致しないため、付属のBase Colorを直接指定する。
    if (!m_albedo.Load("assets/model/Player/Textures/T_Mech_Base_Color.png")) return;

    std::vector<SkinnedVertex> vertices;
    for (const auto& part : m_rig.GetParts())
    {
        for (const auto& source : part.vertices)
        {
            vertices.push_back(MakeVertex(source));
        }
    }
    m_vertices.Create(vertices);
    m_vertexCount = static_cast<unsigned>(vertices.size());

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    static_assert(sizeof(SkinnedVertex) == 52);
    if (FAILED(Renderer::CreateVertexShader(m_vertexShader.GetAddressOf(), m_layout.GetAddressOf(),
        layout, 5, "shader/playerRobotVS.hlsl"))) return;
    if (FAILED(Renderer::CreatePixelShader(m_pixelShader.GetAddressOf(), "shader/litTexturePS.hlsl"))) return;
    if (!Renderer::CreateConstantBuffer(static_cast<unsigned>(sizeof(Matrix) * MaxBones), m_boneBuffer.GetAddressOf())) return;
    m_ready = true;
}

void PlayerRobotModel::Advance(RobotMotion motion, float deltaSeconds, float playbackRate,
                               float upperBodyYaw, float upperBodyPitch, float groundY, bool grounded, bool procedural)
{
    if (!m_ready) return;
    const bool useIK = m_ikReady && procedural;
    m_rig.Update(useIK ? RobotMotion::Idle : motion, deltaSeconds, playbackRate, upperBodyYaw, upperBodyPitch);
    if (useIK) m_spiderIK.Update(m_rig, deltaSeconds, ToAssimp(GetWorldMatrix()), groundY, grounded);
    else m_spiderIK.Reset(); // Legacy clip previews can return to IK without stale planted feet.
}

void PlayerRobotModel::BeginBreakApart(float groundY)
{
    if (!m_ready || m_debris.IsActive()) return;

    // 照準の上下左右と歩行を含む「今の姿勢」を、ワールド座標の破片へ焼き付ける。
    m_debris.Start(m_rig, ToAssimp(GetWorldMatrix()), groundY);
    if (!m_debris.IsActive()) return;
    if (m_debris.GetMatrices().size() > MaxBones)
    {
        m_debris.Reset();
        return;
    }

    std::vector<SkinnedVertex> vertices;
    vertices.reserve(m_debris.GetVertices().size());
    for (const auto& vertex : m_debris.GetVertices()) vertices.push_back(MakeVertex(vertex));
    m_debrisVertices.Create(vertices);
}

void PlayerRobotModel::AdvanceBreakApart(float deltaSeconds)
{
    m_debris.Update(deltaSeconds);
}

void PlayerRobotModel::RestoreBody()
{
    if (!m_debris.IsActive()) return;
    m_debris.Reset();
    m_debrisVertices = VertexBuffer<SkinnedVertex>{};
    m_spiderIK.Reset();
}

Matrix PlayerRobotModel::GetWorldMatrix() const
{
    // 素材の足元はY=0、正面は-Z。判定の底面まで下げ、ゲームの+Zへ正面を合わせる。
    // m_Rotationは脚側の向き。照準へ向ける上半身の回転は骨行列にだけ加えている。
    return Matrix::CreateScale(RobotScale) * Matrix::CreateRotationY(m_Rotation.y + DirectX::XM_PI) *
        Matrix::CreateTranslation(m_Position - Vector3(0, PlayerCollisionRadius, 0));
}

void PlayerRobotModel::Draw(Camera* cam)
{
    if (!m_ready || !cam) return;
    cam->SetCamera();
    // 破片は既にワールド座標。元の機体と二重に描かず、同じ材質・シェーダーで切り替える。
    const bool broken = m_debris.IsActive();
    Matrix world = broken ? Matrix::Identity : GetWorldMatrix();
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    material.TextureEnable = TRUE;
    Renderer::SetMaterial(material);

    std::array<Matrix, MaxBones> matrices;
    matrices.fill(Matrix::Identity);
    const auto& pose = broken ? m_debris.GetMatrices() : m_rig.GetMatrices();
    for (size_t i = 0; i < pose.size(); ++i) matrices[i] = ToDirectX(pose[i]).Transpose();
    auto context = Renderer::GetDeviceContext();
    context->UpdateSubresource(m_boneBuffer.Get(), 0, nullptr, matrices.data(), 0, 0);
    ID3D11Buffer* bones = m_boneBuffer.Get();
    context->VSSetConstantBuffers(6, 1, &bones);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_albedo.SetGPU();
    if (broken) m_debrisVertices.SetGPU();
    else m_vertices.SetGPU();
    context->Draw(m_vertexCount, 0);
    bones = nullptr;
    context->VSSetConstantBuffers(6, 1, &bones);
}

void PlayerRobotModel::Uninit()
{
    m_ready = false;
    m_ikReady = false;
    m_spiderIK = RobotSpiderIK{};
    m_vertexCount = 0;
    m_rig = RobotAnimationRig{};
    m_debris.Reset();
    m_vertices = VertexBuffer<SkinnedVertex>{};
    m_debrisVertices = VertexBuffer<SkinnedVertex>{};
    m_albedo = Texture{};
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_layout.Reset();
    m_boneBuffer.Reset();
}
