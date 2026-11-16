#include "TitleRobotFigure.h"
#include "RobotAnimationRig.h"
#include <assimp/matrix3x3.inl>
#include <assimp/matrix4x4.inl>
#include <assimp/vector3.inl>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

using namespace DirectX::SimpleMath;

namespace {
    constexpr float FigureScale = 0.008f;
    // この固定ポーズの最下点はモデル座標で -8.82。爪を地面へわずかに食い込ませる。
    constexpr float FootOffset = 0.055f;

    Vector3 Position(const aiVector3D& v) {
        return {v.x * FigureScale, v.y * FigureScale + FootOffset, v.z * FigureScale};
    }

    // 点から広がり、先端で細くなる立体の炎。台座の側面・背面からも同じ瞬間が見える。
    void AddFlame(std::vector<VERTEX_3D>& out, Vector3 start, Vector3 end, float radius, const Color& base,
                  const Color& middle, const Color& tip) {
        constexpr int Sides = 8;
        const Vector3 delta = end - start;
        Vector3 side = delta.Cross(Vector3::Up);
        side.Normalize();
        Vector3 up = side.Cross(delta);
        up.Normalize();
        const auto vertex = [](Vector3 p, const Color& color) -> VERTEX_3D {
            return {p, Vector3::Up, color, {0.5f, 0.5f}};
        };
        const auto ring = [&](int i) {
            const float a = i * DirectX::XM_2PI / Sides;
            return vertex(start + delta * 0.27f + radius * (side * std::cos(a) + up * std::sin(a)), middle);
        };
        for (int i = 0; i < Sides; ++i) {
            const auto a = ring(i), b = ring(i + 1);
            out.insert(out.end(), {vertex(start, base), a, b, a, vertex(end, tip), b});
        }
    }

    // 軸はモデル空間の半長。撮影方向から見た長さと太さは頂点シェーダーで投影する。
    void AddGlow(std::vector<VERTEX_3D>& out, Vector3 center, Vector3 halfAxis, float radius, Color color) {
        const Vector2 corners[] = {{-1, -1}, {-1, 1}, {1, 1}, {-1, -1}, {1, 1}, {1, -1}};
        for (auto corner : corners) {
            out.push_back({center, halfAxis, color, corner * radius});
        }
    }

    void AddFlash(std::vector<VERTEX_3D>& vertices, std::vector<VERTEX_3D>& glow,
                  std::vector<VERTEX_3D>& sparks, std::vector<VERTEX_3D>& sparkGlow, Vector3 socket,
                  Vector3 direction, float size, bool tracer) {
        direction.Normalize();
        Vector3 side = direction.Cross(Vector3::Up);
        side.Normalize();
        const Vector3 up = side.Cross(direction);
        const Color white(1, 1, 0.92f, 1), gold(1, 0.85f, 0.31f, 1), orange(1, 0.36f, 0.055f, 1);
        // 輪郭に薄い橙色の光を重ね、中心だけを白熱させる。左・右とも同じ一瞬で固定する。
        AddGlow(glow, socket + direction * size * 0.32f, direction * size * 0.92f, size * 0.49f,
                {1, 0.32f, 0.035f, 0.64f});
        AddGlow(glow, socket + direction * size * 0.18f, direction * size * 0.40f, size * 0.36f,
                {1, 0.66f, 0.16f, 0.93f});
        for (unsigned i = 0; i < 5; ++i) {
            const float a = i * DirectX::XM_2PI / 5.0f + (tracer ? 0.2f : 0.7f);
            const Vector3 radial = side * std::cos(a) + up * std::sin(a);
            const float stretch = 0.37f + 0.11f * (i % 3);
            const auto lobe = size * (direction * stretch + radial * (0.18f + 0.045f * (i % 2)));
            AddGlow(glow, socket + lobe * 0.78f, lobe * 0.63f, size * 0.125f, {1, 0.44f, 0.04f, 0.86f});
            AddFlame(vertices, socket + direction * size * 0.07f, socket + lobe, size * 0.061f, white, gold,
                     orange);
        }
        AddFlame(vertices, socket - direction * 0.025f, socket + direction * size, size * 0.088f, white,
                 white, gold);
        // 小粒の火花を射線の周りに散らす。短い球と長い条を混ぜ、全方向から厚みが読めるようにする。
        for (int i = 0; i < (tracer ? 9 : 6); ++i) {
            const float a = 2.39996f * i + (tracer ? 1.2f : 2.7f);
            const auto radial = side * std::cos(a) + up * std::sin(a);
            const float spread = 0.18f + 0.09f * (i % 4);
            const auto trail = size * (direction * (0.40f + 0.17f * ((3 * i + 1) % 5)) + radial * spread);
            const auto end = socket + trail;
            const auto half = trail * (0.035f + 0.021f * (i % 3));
            AddFlame(sparks, end - half, end + half, size * (0.008f + 0.002f * (i % 3)), orange, gold, white);
            AddGlow(sparkGlow, end, half * 1.8f, size * 0.034f, {1, 0.52f, 0.08f, 0.75f});
        }
    }
} // namespace

bool TitleRobotFigure::Init() {
    RobotAnimationRig rig;
    std::string error;
    if (!rig.Load("assets/model/Player", error)) {
        std::cerr << "Title robot: " << error << '\n';
        return false;
    }
    rig.SetLowerBodyScale(1.15f);
    rig.Update(RobotMotion::Forward, 0.20f, 1.0f, 0.62f, -0.08f);

    const std::string path = "assets/model/Player/Textures/T_Mech_";
    if (!m_albedo.Load(path + "Base_Color.png") || !m_metallic.Load(path + "Metallic.png") ||
        !m_emissive.Load(path + "Emissive.png") || !m_roughness.Load(path + "Roughness.png") ||
        !m_white.Load("assets/texture/white.png")) {
        return false;
    }
    const auto& matrices = rig.GetMatrices();
    std::vector<aiMatrix3x3> normals;
    normals.reserve(matrices.size());
    for (const auto& matrix : matrices) {
        normals.emplace_back(matrix);
        normals.back().Inverse().Transpose();
    }
    std::array<Vector3, 2> flashLights;
    std::vector<VERTEX_3D> effects;
    auto append = [](std::vector<VERTEX_3D>& buffer, const std::vector<VERTEX_3D>& vertices) {
        Range range{static_cast<UINT>(buffer.size()), static_cast<UINT>(vertices.size())};
        buffer.insert(buffer.end(), vertices.begin(), vertices.end());
        return range;
    };
    std::vector<int> gunForNode(matrices.size(), -1);
    // FBXのShotgunTop左右の最前面から計測した上側の銃孔中心（各骨のローカル座標）。
    // 左右は骨の向きが反対。照準回転後の行列で銃口と銃身軸の両方を変換する。
    for (int i = 0; i < static_cast<int>(m_guns.size()); ++i) {
        const bool nearGun = i == 0;
        const int top = rig.FindNode(nearGun ? "ShotgunTop_L" : "ShotgunTop_R");
        const int bottom = rig.FindNode(nearGun ? "ShotgunBot_L" : "ShotgunBot_R");
        if (top < 0 || bottom < 0) {
            std::cerr << "Title robot: missing shotgun joint\n";
            return false;
        }
        const float sign = nearGun ? 1.0f : -1.0f;
        gunForNode[top] = gunForNode[bottom] = i;
        const auto& bone = matrices[top];
        const auto socket = Position(bone * (sign * aiVector3D(43.0614f, -30.5788f, 295.925f)));
        const auto axis = aiMatrix3x3(bone) * aiVector3D(0, 0, sign);
        Vector3 direction(axis.x, axis.y, axis.z);
        direction.Normalize();
        auto& gun = m_guns[i];
        gun.socket = socket;
        gun.axis = direction;
        flashLights[i] = socket + direction * 0.25f;
        const float size = nearGun ? 1.55f : 0.91f;
        std::vector<VERTEX_3D> flash, glow, sparks, sparkGlow, tracer;
        AddFlash(flash, glow, sparks, sparkGlow, socket, direction, size, nearGun);
        if (nearGun) {
            AddFlame(tracer, socket + direction * (size + 0.26f), socket + direction * (size + 0.85f), 0.040f,
                     {1, 0.36f, 0.055f, 1}, {1, 0.85f, 0.31f, 1}, {1, 1, 0.92f, 1});
        }
        gun.flash = append(effects, flash);
        gun.glow = append(effects, glow);
        gun.sparks = append(effects, sparks);
        gun.sparkGlow = append(effects, sparkGlow);
        gun.tracer = append(effects, tracer);
    }

    std::vector<VERTEX_3D> robot;
    std::array<std::vector<VERTEX_3D>, 2> barrels;
    auto bakeVertex = [&](const RobotAnimationRig::Vertex& v) -> VERTEX_3D {
        const auto p = Position(matrices[v.node] * v.position);
        const auto n = (normals[v.node] * v.normal).Normalize();
        const Vector3 normal(n.x, n.y, n.z);
        float fireLight = 0;
        for (const auto& light : flashLights) {
            const Vector3 toLight = light - p;
            const float d2 = (std::max)(0.01f, toLight.LengthSquared());
            const float cosine = (std::max)(0.0f, normal.Dot(toLight) / std::sqrt(d2));
            fireLight += (0.20f + 0.8f * cosine) / (1.0f + 2.3f * d2);
        }
        return {p, normal, {1, 1, 1, fireLight}, {v.uv.x, v.uv.y}};
    };
    for (const auto& part : rig.GetParts()) {
        // 三角形を分断せず、銃身だけを射線の反対方向に動かす。
        for (size_t first = 0; first < part.vertices.size(); first += 3) {
            const int gun = gunForNode[part.vertices[first].node];
            auto& vertices = gun < 0 ? robot : barrels[gun];
            for (size_t corner = 0; corner < 3; ++corner) {
                vertices.push_back(bakeVertex(part.vertices[first + corner]));
            }
        }
    }
    m_bodyCount = static_cast<UINT>(robot.size());
    for (size_t i = 0; i < m_guns.size(); ++i) {
        m_guns[i].barrel = append(robot, barrels[i]);
    }
    m_robot.Create(robot);
    m_effects.Create(effects);
    m_robotCount = static_cast<UINT>(robot.size());
    Renderer::CreateConstantBuffer(sizeof(Vector4), m_lightPulse.GetAddressOf());
    m_robotShader.Create("shader/titleDioramaVS.hlsl", "shader/titleRobotFigurePS.hlsl");
    m_flashShader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
    m_glowShader.Create("shader/titleMuzzleGlowVS.hlsl", "shader/titleMuzzleGlowPS.hlsl");
    return true;
}

void TitleRobotFigure::DrawBodyDepth(const Matrix& world, float recoil) {
    if (!m_robotCount) {
        return;
    }
    auto context = Renderer::GetDeviceContext();
    Matrix transform = world;
    Renderer::SetWorldMatrix(&transform);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_robot.SetGPU();
    if (recoil <= 0) {
        context->Draw(m_robotCount, 0);
        return;
    }
    context->Draw(m_bodyCount, 0);
    for (const auto& gun : m_guns) {
        transform = Matrix::CreateTranslation(-gun.axis * recoil) * world;
        Renderer::SetWorldMatrix(&transform);
        context->Draw(gun.barrel.count, gun.barrel.first);
    }
}

void TitleRobotFigure::Draw(const Matrix& world, const FirePose& pose) {
    if (!m_robotCount) {
        return;
    }
    auto context = Renderer::GetDeviceContext();
    m_robotShader.SetGPU();
    const std::array<ID3D11ShaderResourceView*, 4> maps = {
        m_albedo.GetShaderResourceView(), m_metallic.GetShaderResourceView(),
        m_emissive.GetShaderResourceView(), m_roughness.GetShaderResourceView()};
    context->PSSetShaderResources(0, static_cast<UINT>(maps.size()), maps.data());
    const float t = pose.shotSeconds;
    // 凍結した炎を短く膨らませて消し、火花と弾は銃身から独立して飛ばす。
    const float flash =
        (1.0f + 0.80f * std::clamp(t / 0.035f, 0.0f, 1.0f)) * std::clamp(1.0f - t / 0.18f, 0.0f, 1.0f);
    if (m_lightPulse) {
        const Vector4 pulse(pose.sensor, flash - 1.0f, 0, 0);
        context->UpdateSubresource(m_lightPulse.Get(), 0, nullptr, &pulse, 0, 0);
    }
    context->PSSetConstantBuffers(8, 1, m_lightPulse.GetAddressOf());
    DrawBodyDepth(world, pose.recoil);
    ID3D11Buffer* emptyBuffer = nullptr;
    context->PSSetConstantBuffers(8, 1, &emptyBuffer);

    ID3D11ShaderResourceView* empty[3]{};
    context->PSSetShaderResources(1, 3, empty);
    m_flashShader.SetGPU();
    m_white.SetGPU();
    m_effects.SetGPU();
    auto drawBursts = [&](bool halo) {
        for (const auto& gun : m_guns) {
            auto draw = [&](Range range, float scale, float travel) {
                if (range.count && scale > 0) {
                    // 拡縮の中心は各銃口に置き、離れた粒子は射線に沿って送る。
                    Matrix transform =
                        Matrix::CreateScale(scale) *
                        Matrix::CreateTranslation(gun.socket * (1.0f - scale) + gun.axis * travel) * world;
                    Renderer::SetWorldMatrix(&transform);
                    context->Draw(range.count, range.first);
                }
            };
            draw(halo ? gun.glow : gun.flash, flash, -pose.recoil);
            const float sparkLife = std::clamp((0.30f - t) / 0.11f, 0.0f, 1.0f);
            draw(halo ? gun.sparkGlow : gun.sparks, (1.0f + t * 2.4f) * sparkLife,
                 t * 10.0f + (1.0f - sparkLife));
            if (!halo) {
                draw(gun.tracer, 1.0f + 0.85f * std::clamp(t / 0.045f, 0.0f, 1.0f), t * 38.0f);
            }
        }
    };
    drawBursts(false);
    Renderer::SetDepthEnable(false);
    Renderer::SetBlendState(BS_ADDITIVE);
    m_glowShader.SetGPU();
    drawBursts(true);
    Renderer::SetBlendState(BS_NONE);
    Renderer::SetDepthEnable(true);
}
