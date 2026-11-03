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
    // FBX内の画像参照は配布フォルダと一致しないため、付属のBase Colorを直接指定する。
    if (!m_albedo.Load("assets/model/Player/Textures/T_Mech_Base_Color.png")) return;

    std::vector<SkinnedVertex> vertices;
    for (const auto& part : m_rig.GetParts())
    {
        for (const auto& source : part.vertices)
        {
            SkinnedVertex destination{};
            destination.vertex.position = Vector3(source.position.x, source.position.y, source.position.z);
            destination.vertex.normal = Vector3(source.normal.x, source.normal.y, source.normal.z);
            destination.vertex.uv = Vector2(source.uv.x, source.uv.y);
            destination.vertex.color = Color(1, 1, 1, 1);
            destination.bone = source.node;
            vertices.push_back(destination);
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

void PlayerRobotModel::Advance(RobotMotion motion, float deltaSeconds, float playbackRate)
{
    if (m_ready) m_rig.Update(motion, deltaSeconds, playbackRate);
}

void PlayerRobotModel::Draw(Camera* cam)
{
    if (!m_ready || !cam) return;
    cam->SetCamera();
    // 素材の足元はY=0、正面は-Z。判定の底面まで下げ、ゲームの+Zへ正面を合わせる。
    Matrix world = Matrix::CreateScale(RobotScale) * Matrix::CreateRotationY(m_Rotation.y + DirectX::XM_PI) *
        Matrix::CreateTranslation(m_Position - Vector3(0, PlayerCollisionRadius, 0));
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    material.TextureEnable = TRUE;
    Renderer::SetMaterial(material);

    std::array<Matrix, MaxBones> matrices;
    matrices.fill(Matrix::Identity);
    const auto& pose = m_rig.GetMatrices();
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
    m_vertices.SetGPU();
    context->Draw(m_vertexCount, 0);
    bones = nullptr;
    context->VSSetConstantBuffers(6, 1, &bones);
}

void PlayerRobotModel::Uninit()
{
    m_ready = false;
    m_vertexCount = 0;
    m_rig = RobotAnimationRig{};
    m_vertices = VertexBuffer<SkinnedVertex>{};
    m_albedo = Texture{};
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_layout.Reset();
    m_boneBuffer.Reset();
}
