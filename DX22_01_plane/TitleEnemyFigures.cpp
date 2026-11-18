/// @brief ゲームの敵モデルを静止した攻撃ポーズで展示する。
#include "TitleEnemyFigures.h"
#include "BoarAnimationRig.h"
#include "FbxImportSettings.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>

using namespace DirectX::SimpleMath;

namespace {
    Vector3 ToVector(const aiVector3D& v) {
        return {v.x, v.y, v.z};
    }

    class FigureMesh : public Mesh {
    public:
        // Monster Packの上方向は-Z。Ghostはノード変換を含めると通常のY-up。
        bool LoadFbx(const char* path, bool monsterPack) {
            Assimp::Importer importer;
            // 展示メッシュが下で素材の軸を合わせるので、インポーターによる二重回転を避ける。
            ConfigureFbxImport(importer);
            unsigned flags = aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                             aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals |
                             aiProcess_JoinIdenticalVertices;
            if (monsterPack) {
                // 同梱FBXは各三角形に平面の法線を持つ。鋭い折れ目を残して展示用の法線を作り直す。
                importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 75.0f);
                flags |= aiProcess_ForceGenNormals;
            }
            const auto* scene = importer.ReadFile(path, flags);
            if (!scene) {
                std::cerr << "Title figure " << path << ": " << importer.GetErrorString() << '\n';
                return false;
            }
            for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
                const auto& mesh = *scene->mMeshes[i];
                if (!mesh.HasPositions() || !mesh.HasNormals()) {
                    continue;
                }
                aiColor4D diffuse(1, 1, 1, 1);
                if (!monsterPack) {
                    scene->mMaterials[mesh.mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
                }
                const unsigned base = static_cast<unsigned>(m_vertices.size());
                for (unsigned j = 0; j < mesh.mNumVertices; ++j) {
                    const auto& p = mesh.mVertices[j];
                    const auto& n = mesh.mNormals[j];
                    const auto uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][j] : aiVector3D{};
                    m_vertices.push_back({monsterPack ? Vector3(p.x, -p.z, p.y) : ToVector(p),
                                          monsterPack ? Vector3(n.x, -n.z, n.y) : ToVector(n),
                                          Color(diffuse.r, diffuse.g, diffuse.b, 1), Vector2(uv.x, uv.y)});
                }
                for (unsigned j = 0; j < mesh.mNumFaces; ++j) {
                    const auto& face = mesh.mFaces[j];
                    if (face.mNumIndices == 3) {
                        for (unsigned k = 0; k < 3; ++k) {
                            m_indices.push_back(base + face.mIndices[k]);
                        }
                    }
                }
            }
            return Fit();
        }

        // 本編の突進アニメの1コマを読み込み時に焼き込み、展示中は骨を更新しない。
        bool LoadChargingBoar(Texture& albedo) {
            BoarAnimationRig rig;
            std::string error;
            if (!rig.Load("assets/model/Boar/Boar.glb", error)) {
                std::cerr << "Title boar: " << error << '\n';
                return false;
            }
            rig.Update(BoarMotion::Charge, 0.2f, 0.55f);
            const auto& matrices = rig.GetMatrices();
            const auto& vertices = rig.GetVertices();
            m_vertices.reserve(vertices.size());
            m_indices.reserve(vertices.size());
            for (const auto& v : vertices) {
                aiVector3D position, normal;
                for (size_t j = 0; j < v.weights.size(); ++j) {
                    if (v.weights[j] == 0) {
                        continue;
                    }
                    const auto& bone = matrices[v.bones[j]];
                    position += (bone * v.position) * v.weights[j];
                    normal += (aiMatrix3x3(bone) * v.normal) * v.weights[j];
                }
                normal.NormalizeSafe();
                m_indices.push_back(static_cast<unsigned>(m_vertices.size()));
                m_vertices.push_back(
                    {ToVector(position), ToVector(normal), Color(0.8f, 0.8f, 0.8f, 1), {v.uv.x, v.uv.y}});
            }
            const auto& embedded = rig.GetAlbedo();
            return Fit() && !embedded.empty() &&
                   albedo.LoadFromFemory(embedded.data(), static_cast<int>(embedded.size()));
        }

    private:
        // 最大寸法1、足元中央原点に揃え、配置の数値を台座上の寸法で指定できるようにする。
        bool Fit() {
            if (m_vertices.empty() || m_indices.empty()) {
                return false;
            }
            Vector3 low = m_vertices.front().position, high = low;
            for (const auto& v : m_vertices) {
                low = Vector3::Min(low, v.position);
                high = Vector3::Max(high, v.position);
            }
            const Vector3 span = high - low;
            const float extent = (std::max)({span.x, span.y, span.z});
            if (extent <= 0) {
                return false;
            }
            const Vector3 feet((low.x + high.x) * 0.5f, low.y, (low.z + high.z) * 0.5f);
            for (auto& v : m_vertices) {
                v.position = (v.position - feet) / extent;
            }
            return true;
        }
    };

    Matrix TowardRobot(float x, float altitude, float z, Vector3 scale, float pitch = 0, float bank = 0,
                       Vector2 aim = Vector2::Zero) {
        // 各素材の正面は-Z。まず自身の軸で傾けてから中心へ向ける。
        const float yaw = std::atan2(x - aim.x, z - aim.y);
        return Matrix::CreateScale(scale) * Matrix::CreateRotationZ(bank) * Matrix::CreateRotationX(pitch) *
               Matrix::CreateRotationY(yaw) * Matrix::CreateTranslation(x, altitude, z);
    }
} // namespace

bool TitleEnemyFigures::Init(const std::function<float(float, float)>& groundHeight) {
    using enum TitleEnemyKind;
    m_portraitShader.Create("shader/titleDioramaVS.hlsl", "shader/titleEnemyPortraitPS.hlsl");
    Renderer::CreateConstantBuffer(sizeof(Vector4) * 2, m_portraitLight.GetAddressOf());
    // 左手前のドローンの胴を銃口の射線(-2, 3.2, -6.7)に置く。
    m_placements = {{{Drone, TowardRobot(-2.0f, 2.05f, -6.7f, {3.1f, 3.1f, 3.1f}, -0.23f, 0.35f)},
                     {Drone, TowardRobot(-5.8f, 5.5f, -1.8f, {2.45f, 2.45f, 2.45f}, -0.40f, -0.35f)},
                     {Ghost, TowardRobot(-0.8f, 3.5f, 6.3f, {3.1f, 3.1f, 3.1f}, -0.15f, -0.12f)},
                     // 外側から手前の脚を狙わせ、イノシシの牙とスライムの顔を斜めから見せる。
                     {Boar, TowardRobot(2.2f, groundHeight(2.2f, 7.0f), 7.0f, {4.25f, 4.25f, 4.25f}, 0, 0,
                                        {1.3f, -1.2f})},
                     {Slime, TowardRobot(-2.12f, groundHeight(-2.12f, -7.25f), -7.25f, {2.55f, 2.1f, 2.7f},
                                         0.06f, 0, {2.4f, -5.2f})},
                     // 少し見上げる姿勢で、開始時の俯瞰でも傘の下に両目が見える。
                     {Mushroom, TowardRobot(7.8f, groundHeight(7.8f, 0.8f), 0.8f, {2.35f, 2.35f, 2.35f},
                                            0.28f, 0, {7.8f, -5.3f})}}};
    auto prepare = [&](TitleEnemyKind kind, FigureMesh& mesh) {
        m_figures[static_cast<size_t>(kind)].renderer.Init(mesh);
        // 実際に描く姿勢で頂点を測る。翼や牙を含む投影範囲を収め、読み込み後は再計算しない。
        for (auto& placement : m_placements) {
            if (placement.kind != kind) {
                continue;
            }
            auto& target = placement.target;
            target.toEye = Vector3::TransformNormal({0.36f, 0.24f, -1.0f}, placement.local);
            target.toEye.Normalize();
            Vector3 right = Vector3::Up.Cross(-target.toEye);
            right.Normalize();
            const Vector3 up = (-target.toEye).Cross(right);
            Vector3 low(FLT_MAX), high(-FLT_MAX);
            for (const auto& vertex : mesh.GetVertices()) {
                const auto point = Vector3::Transform(vertex.position, placement.local);
                const Vector3 projected(point.Dot(right), point.Dot(up), point.Dot(target.toEye));
                low = Vector3::Min(low, projected);
                high = Vector3::Max(high, projected);
            }
            const auto mid = (high + low) * 0.5f;
            target.center = right * mid.x + up * mid.y + target.toEye * mid.z;
            target.size = {high.x - low.x, high.y - low.y};
            placement.radius = (high - low).Length() * 0.5f;
        }
    };
    auto loadFbx = [&](TitleEnemyKind kind, const char* path, const char* texture) {
        FigureMesh mesh;
        auto& figure = m_figures[static_cast<size_t>(kind)];
        const bool monsterPack = kind == Drone || kind == Slime || kind == Mushroom;
        figure.ready = mesh.LoadFbx(path, monsterPack) && figure.texture.Load(texture);
        if (figure.ready) {
            prepare(kind, mesh);
        }
    };
    loadFbx(Drone, "assets/model/drone/Drone.fbx", "assets/model/drone/albedo.tga");
    loadFbx(Slime, "assets/model/slime/Slime.fbx", "assets/model/slime/albedo.tga");
    loadFbx(Ghost, "assets/model/Ghost/Ghost.fbx", "assets/texture/white.png");
    loadFbx(Mushroom, "assets/model/mushroom/Mushroom.fbx", "assets/model/mushroom/albedo.tga");
    FigureMesh boar;
    auto& charging = m_figures[static_cast<size_t>(Boar)];
    charging.ready = boar.LoadChargingBoar(charging.texture);
    if (charging.ready) {
        prepare(Boar, boar);
    }

    return std::all_of(m_figures.begin(), m_figures.end(), [](const Figure& figure) { return figure.ready; });
}

const TitleEnemyCamera::Target* TitleEnemyFigures::GetTarget(TitleEnemyKind kind) const {
    for (const auto& placement : m_placements) {
        if (placement.kind == kind && placement.radius > 0) {
            return &placement.target;
        }
    }
    return nullptr;
}

void TitleEnemyFigures::Draw(const Matrix& root, const TitleEnemyCamera& camera, TitleEnemyKind selected,
                             bool shadowPass) {
    const auto* target = GetTarget(selected);
    const bool inspectionLight = camera.InspectionWeight() > 0 && !shadowPass && m_portraitLight;
    auto* context = Renderer::GetDeviceContext();
    if (inspectionLight) {
        m_portraitShader.SetGPU();
        context->PSSetConstantBuffers(9, 1, m_portraitLight.GetAddressOf());
    }
    for (const auto& placement : m_placements) {
        auto& figure = m_figures[static_cast<size_t>(placement.kind)];
        const bool subject = &placement.target == target;
        const float fade = subject || !camera.Active()
                               ? 0.0f
                               : camera.OtherFigureFade(Vector3::Transform(placement.target.center, root));
        if (!figure.ready || fade >= (shadowPass ? 0.5f : 1.0f)) {
            continue;
        }
        if (inspectionLight) {
            const auto eye = camera.EyeDirection();
            const Vector4 light[] = {{eye.x, eye.y, eye.z, camera.InspectionWeight()}, {fade, 0, 0, 0}};
            context->UpdateSubresource(m_portraitLight.Get(), 0, nullptr, light, 0, 0);
        }
        Matrix world = placement.local * root;
        Renderer::SetWorldMatrix(&world);
        figure.texture.SetGPU();
        figure.renderer.Draw();
    }
    if (inspectionLight) {
        ID3D11Buffer* unbound = nullptr;
        context->PSSetConstantBuffers(9, 1, &unbound);
    }
}
