/// @brief 敵に包囲されたロボットの射撃を切り取った、回転式の展示台。
#include "TitleScene.h"
#include "Application.h"
#include "Game.h"
#include "GameRules.h"
#include "GameSettings.h"
#include "GameTime.h"
#include "FbxImportSettings.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
#include "TitleEnemyFigures.h"
#include "TitleFigureShadows.h"
#include "TitleRobotFigure.h"
#include "imgui.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace DirectX::SimpleMath;

namespace {
    const std::string Root = "assets/model/Title/";
    // 台座上のロボットの展示倍率。接地影と遮蔽判定にも同じ倍率を使う。
    constexpr float RobotDisplayScale = 0.70f;
    constexpr float StartShotSeconds = 0.20f;
    constexpr float StartFadeSeconds = 0.48f;
    constexpr float StartDuration = 0.80f;

    float SmoothProgress(float seconds, float duration) {
        const float t = std::clamp(seconds / duration, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    const char* Text(const char8_t* value) {
        return reinterpret_cast<const char*>(value);
    }

    // 展示台が収まる正射影の高さ（ワールド単位）。ドラッグの感度も同じ縮尺に合わせる。
    float ViewHeight(float aspect) {
        return TitleEnemyCamera::HomeViewHeight(aspect);
    }

    // 既存のゲーム用ローダーを変更せず、ノード変換と画像の割り当てを展示側で処理する。
    class DisplayMesh : public Mesh {
    public:
        struct Part {
            unsigned first, count, material;
        };

        std::vector<Part> parts;
        MeshRenderer renderer;

        bool Load(const std::string& path, bool island, bool grass) {
            Assimp::Importer importer;
            ConfigureFbxImport(importer);
            // 草の異なるLODを同じ材質だからと結合しない。
            importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
            const auto scene = importer.ReadFile(
                Root + path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                 aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals);
            if (!scene || !scene->mNumMeshes) {
                std::cerr << "Title model: " << path << ": " << importer.GetErrorString() << '\n';
                return false;
            }
            unsigned selected = 0;
            if (grass) {
                for (unsigned i = 1; i < scene->mNumMeshes; ++i) {
                    if (scene->mMeshes[i]->mNumFaces > scene->mMeshes[selected]->mNumFaces) {
                        selected = i;
                    }
                }
            }
            for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
                if (grass && i != selected) {
                    continue;
                }
                const auto mesh = scene->mMeshes[i];
                const unsigned base = static_cast<unsigned>(m_vertices.size());
                const unsigned first = static_cast<unsigned>(m_indices.size());
                for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
                    const auto p = mesh->mVertices[v], n = mesh->mNormals[v];
                    const auto uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D{};
                    VERTEX_3D vertex{};
                    vertex.position = Vector3(p.x, p.y, p.z);
                    vertex.normal = Vector3(n.x, n.y, n.z);
                    vertex.uv = Vector2(uv.x, uv.y);
                    vertex.color = grass ? Color(0.40f, 0.54f, 0.21f, 1) : Color(1, 1, 1, 1);
                    m_vertices.push_back(vertex);
                }
                for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
                    const auto& face = mesh->mFaces[f];
                    if (face.mNumIndices == 3) {
                        for (unsigned j = 0; j < 3; ++j) {
                            m_indices.push_back(base + face.mIndices[j]);
                        }
                    }
                }
                parts.push_back(
                    {first, static_cast<unsigned>(m_indices.size()) - first, mesh->mMaterialIndex});
            }
            if (m_vertices.empty() || m_indices.empty()) {
                return false;
            }
            Vector3 low = m_vertices[0].position, high = low;
            for (const auto& v : m_vertices) {
                low = Vector3::Min(low, v.position);
                high = Vector3::Max(high, v.position);
            }
            const Vector3 center((low.x + high.x) * 0.5f, low.y, (low.z + high.z) * 0.5f);
            const float width = (std::max)(high.x - low.x, high.z - low.z);
            if (width < 0.001f) {
                return false;
            }
            for (auto& v : m_vertices) {
                if (island) {
                    // 岩の断面を残しつつ浮島の先端を圧縮し、上面をほぼY=0にする。
                    v.position.x = (v.position.x - center.x) * 20.0f / width;
                    v.position.z = (v.position.z - center.z) * 20.0f / width;
                    v.position.y = (v.position.y - high.y) * 0.43f;
                    // 埋め込んだ島の先端が展示台の底から突き抜けないようにする。
                    v.position.y = (std::max)(v.position.y, -1.55f);
                    if (v.position.y < -1.12f) {
                        const float radius = std::hypot(v.position.x, v.position.z);
                        if (radius > 10.06f) {
                            v.position.x *= 10.06f / radius;
                            v.position.z *= 10.06f / radius;
                        }
                    }
                    v.normal.y /= 0.43f;
                    v.normal.Normalize();
                } else {
                    v.position = (v.position - center) / width;
                }
            }
            renderer.Init(*this);
            return true;
        }

        // 上面の三角形を補間し、凹凸の上に小物を接地する。
        float Height(float x, float z) const {
            float result = -100.0f;
            for (size_t i = 0; i + 2 < m_indices.size(); i += 3) {
                const auto& a = m_vertices[m_indices[i]].position;
                const auto& b = m_vertices[m_indices[i + 1]].position;
                const auto& c = m_vertices[m_indices[i + 2]].position;
                const float d = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
                if (std::abs(d) < 0.00001f) {
                    continue;
                }
                const float u = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
                const float v = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
                if (u >= 0 && v >= 0 && u + v <= 1) {
                    result = (std::max)(result, u * a.y + v * b.y + (1 - u - v) * c.y);
                }
            }
            return result;
        }
    };

    class ColorMesh : public Mesh {
    public:
        // 旋盤で削った展示台の断面。上から順に天板、細い真鍮の縁、黒い側面、脚。
        void Plinth() {
            struct Ring {
                float radius, y;
                Color finish;
            };

            const Color charcoal(0.095f, 0.13f, 0.15f, 1);
            const Color brushed(0.25f, 0.32f, 0.33f, 1);
            const Color brass(0.64f, 0.43f, 0.21f, 1);
            const Ring profile[] = {{0, -1.14f, charcoal},      {10.10f, -1.14f, brushed},
                                    {10.23f, -1.23f, charcoal}, {10.23f, -1.32f, brass},
                                    {10.23f, -1.36f, charcoal}, {10.15f, -1.42f, charcoal},
                                    {10.15f, -2.06f, brushed},  {10.03f, -2.16f, charcoal},
                                    {9.82f, -2.16f, charcoal},  {9.82f, -2.26f, charcoal}};
            constexpr unsigned Sides = 128;
            for (unsigned row = 0; row + 1 < std::size(profile); ++row) {
                const auto& top = profile[row];
                const auto& bottom = profile[row + 1];
                const unsigned base = static_cast<unsigned>(m_vertices.size());
                for (unsigned i = 0; i <= Sides; ++i) {
                    const float angle = i * DirectX::XM_2PI / Sides;
                    const float x = std::cos(angle), z = std::sin(angle);
                    Vector3 normal(x * (top.y - bottom.y), bottom.radius - top.radius,
                                   z * (top.y - bottom.y));
                    normal.Normalize();
                    for (auto ring : {top, bottom}) {
                        m_vertices.push_back(
                            {{x * ring.radius, ring.y, z * ring.radius}, normal, top.finish, {}});
                    }
                    if (i == 0) {
                        continue;
                    }
                    const unsigned a = base + (i - 1) * 2;
                    m_indices.insert(m_indices.end(), {a, a + 2, a + 1, a + 1, a + 2, a + 3});
                }
            }
        }

        void Backdrop() {
            m_vertices = {{{-1, -1, 0.999f}, {}, {1, 1, 1, 1}, {0, 1}},
                          {{-1, 1, 0.999f}, {}, {1, 1, 1, 1}, {0, 0}},
                          {{1, 1, 0.999f}, {}, {1, 1, 1, 1}, {1, 0}},
                          {{1, -1, 0.999f}, {}, {1, 1, 1, 1}, {1, 1}}};
            m_indices = {0, 1, 2, 0, 2, 3};
        }

        void PlinthShadow() {
            constexpr unsigned Sides = 64;
            const Vector2 falloff[] = {{0, 0.68f}, {9.7f, 0.67f}, {10.6f, 0.3f}, {12.1f, 0.055f}, {13.6f, 0}};
            for (const auto ring : falloff) {
                for (unsigned i = 0; i <= Sides; ++i) {
                    const float a = i * DirectX::XM_2PI / Sides;
                    m_vertices.push_back({{ring.x * std::cos(a), -2.27f, ring.x * std::sin(a)},
                                          {},
                                          {0.018f, 0.024f, 0.03f, ring.y},
                                          {}});
                }
            }
            for (unsigned r = 0; r + 1 < std::size(falloff); ++r) {
                for (unsigned i = 0; i < Sides; ++i) {
                    const unsigned a = r * (Sides + 1) + i, b = a + Sides + 1;
                    m_indices.insert(m_indices.end(), {a, a + 1, b, b, a + 1, b + 1});
                }
            }
        }

        void ContactShadow(const DisplayMesh& ground, float x, float z, float radius) {
            const unsigned base = static_cast<unsigned>(m_vertices.size());
            constexpr unsigned Segments = 24;
            m_vertices.push_back(
                {{x, ground.Height(x, z) + 0.025f, z}, {}, {0.06f, 0.08f, 0.06f, 0.26f}, {0.5f, 0.5f}});
            for (unsigned i = 0; i <= Segments; ++i) {
                const float a = DirectX::XM_2PI * i / Segments;
                const float px = x + std::cos(a) * radius, pz = z + std::sin(a) * radius;
                const float y = ground.Height(px, pz);
                m_vertices.push_back(
                    {{px, (std::max)(y, -0.8f) + 0.025f, pz}, {}, {0.06f, 0.08f, 0.06f, 0}, {0.5f, 0.5f}});
                if (i) {
                    m_indices.insert(m_indices.end(), {base, base + i, base + i + 1});
                }
            }
        }
    };
} // namespace

struct TitleScene::Diorama {
    struct Asset {
        DisplayMesh mesh;
        std::vector<std::shared_ptr<Texture>> textures;
        bool ready = false;
    };

    struct Prop {
        unsigned asset;
        Matrix local;
        Vector3 center;
        float radius;
    };

    enum {
        Island,
        Rock,
        PlantA,
        PlantB,
        Grass,
        Branch,
        Metal,
        AssetCount
    };

    Asset assets[AssetCount];
    std::vector<Prop> props;
    Shader surface, groundShader, backdropShader, shadowShader;
    Texture white, gravel;
    TitleEnemyFigures enemies;
    TitleEnemyCamera enemyCamera;
    TitleEnemyKind selected = TitleEnemyKind::Count;
    TitleFigureShadows castShadows;
    TitleRobotFigure robot;
    Matrix robotLocal;
    MeshRenderer backdrop, shadows, plinth;
    bool hasShadows = false;
    ComPtr<ID3D11RasterizerState> twoSided;
    ComPtr<ID3D11DepthStencilState> noDepth;
    // 開始時はセンサーと左右の銃口が同時に見える斜め正面。
    float yaw = 0.086f;
    /// 展示部分で左ボタンを押してから離すまで、自動回転を止める。
    bool m_dragging = false;
    /// ゲーム開始が押されてからの秒数。鑑賞中は空。
    std::optional<float> m_startElapsed;
    enum class MenuPage {
        Settings,
        Rules
    };
    MenuPage m_page = MenuPage::Settings;
    GameRules m_rules;

    void FocusEnemy(TitleEnemyKind kind) {
        if (const auto* target = enemies.GetTarget(kind)) {
            m_dragging = false;
            selected = kind;
            enemyCamera.Focus(*target, Matrix::CreateRotationY(yaw));
        }
    }

    void LeaveEnemyView() {
        enemyCamera.Leave();
    }

    bool IsInspectingEnemy() const {
        return enemyCamera.Active();
    }

    void UpdateCamera(float dt) {
        enemyCamera.Update(dt);
        if (!enemyCamera.Active()) {
            selected = TitleEnemyKind::Count;
        }
    }

    Diorama() {
        white.Load("assets/texture/white.png");
        gravel.Load("assets/model/cylinder/gravel.jpg");
        surface.Create("shader/titleDioramaVS.hlsl", "shader/titleDioramaPS.hlsl");
        groundShader.Create("shader/titleDioramaVS.hlsl", "shader/titleGroundPS.hlsl");
        backdropShader.Create("shader/unlitTextureVS.hlsl", "shader/titleBackdropPS.hlsl");
        shadowShader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
        D3D11_RASTERIZER_DESC raster{};
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
        raster.DepthClipEnable = TRUE;
        raster.MultisampleEnable = TRUE;
        Renderer::GetDevice()->CreateRasterizerState(&raster, twoSided.GetAddressOf());
        D3D11_DEPTH_STENCIL_DESC depth{};
        depth.DepthEnable = FALSE;
        depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
        Renderer::GetDevice()->CreateDepthStencilState(&depth, noDepth.GetAddressOf());
        ColorMesh background;
        background.Backdrop();
        backdrop.Init(background);
        ColorMesh displayStand;
        displayStand.Plinth();
        plinth.Init(displayStand);
        auto texture = [](const std::string& path) {
            auto result = std::make_shared<Texture>();
            return result->Load(Root + path) ? result : std::shared_ptr<Texture>{};
        };
        const auto nature = texture("stylizednaturelite_fbx/Unity/All_lp_All_01_BaseMap.png");
        const auto wood = texture("stylizednaturelite_fbx/Unity/Tree_01_lp_Tree_BaseMap.png");
        auto load = [&](unsigned id, const std::string& path, std::vector<std::shared_ptr<Texture>> images) {
            assets[id].ready = assets[id].mesh.Load(path, id == Island, id == Grass);
            assets[id].textures = std::move(images);
        };
        // 画像参照のないFBXにも、配布画像を明示的に対応付ける。
        load(Island, "floating-island/source/model/model/model.fbx",
             {texture("floating-island/textures/phong2SG_albedo.jpeg"),
              texture("floating-island/textures/phong1SG_albedo.jpeg")});
        load(Rock, "stylizednaturelite_fbx/SM_Rock_01.fbx", {nature});
        load(PlantA, "stylizednaturelite_fbx/SM_Plant_01.fbx", {nature});
        load(PlantB, "stylizednaturelite_fbx/SM_Plant_02.fbx", {nature});
        load(Grass, "grass_patch_3_lods/source/GrassPatch.fbx", {});
        load(Branch, "stylizednaturelite_fbx/SM_Branch_01.fbx", {wood});
        const std::string metal = "urban_shanty_town_sheet_metal_corrugated_03_xmufdce_low/";
        load(Metal, metal + "Urban_Shanty_Town_Sheet_Metal_Corrugated_03_xmufdce_Low.fbx",
             {texture(metal + "Urban_Shanty_Town_Sheet_Metal_Corrugated_03_xmufdce_Low_1K_BaseColor.jpg")});
        ColorMesh contact;
        if (!castShadows.Init()) {
            std::cerr << "Title shadow map unavailable\n";
        }
        enemies.Init([&](float x, float z) { return assets[Island].mesh.Height(x, z); });
        robot.Init();
        robotLocal = Matrix::CreateScale(RobotDisplayScale) *
                     Matrix::CreateTranslation(0, assets[Island].mesh.Height(0, 0), 0);
        contact.PlinthShadow();
        contact.ContactShadow(assets[Island].mesh, 0, 0, 3.9f * RobotDisplayScale);
        auto place = [&](unsigned asset, float x, float z, Vector3 scale, float rotation,
                         float sink = 0.02f) {
            const float y = assets[Island].mesh.Height(x, z);
            if (y < -1.0f || !assets[asset].ready) {
                return;
            }
            props.push_back({asset,
                             Matrix::CreateScale(scale) * Matrix::CreateRotationY(rotation) *
                                 Matrix::CreateTranslation(x, y - sink, z),
                             {x, y - sink + scale.y * 0.25f, z},
                             scale.Length() * 0.5f});
            if (asset == Rock || asset == Metal || asset == Branch) {
                contact.ContactShadow(assets[Island].mesh, x, z, scale.x * 0.52f);
            }
        };
        // 中央の射撃姿勢を見渡せるよう、外周に大小の岩をまとめる。
        place(Rock, -5.9f, 3.8f, {2.9f, 1.8f, 2.6f}, 0.2f, 0.24f);
        place(Rock, -4.3f, 5.5f, {2.1f, 1.3f, 1.8f}, 1.7f, 0.17f);
        place(Rock, -6.6f, 1.9f, {1.5f, 0.9f, 1.5f}, 2.4f, 0.12f);
        place(Rock, 5.8f, -3.9f, {2.3f, 1.3f, 2.1f}, 1.2f, 0.18f);
        place(Rock, 6.6f, -1.8f, {1.5f, 0.9f, 1.3f}, 2.8f, 0.12f);
        place(Rock, 3.8f, -5.7f, {1.1f, 0.7f, 1.2f}, 0.6f, 0.09f);
        place(Rock, 0.9f, 6.7f, {1.0f, 0.65f, 1.2f}, 1.3f, 0.08f);
        place(Metal, 4.2f, 3.6f, {4.1f, 4.1f, 4.1f}, -0.65f, 0.01f);
        place(Metal, 5.2f, 4.6f, {2.6f, 2.6f, 2.6f}, 0.65f, -0.06f);
        place(Branch, -0.6f, -6.0f, {3.4f, 3.4f, 3.4f}, 0.65f);
        const Vector2 clumps[] = {{-6, 4.9f},    {-4.2f, 4.3f}, {-6.4f, 2.6f}, {-5.8f, -3.4f},
                                  {4.7f, -4.9f}, {5.7f, -4.4f}, {6.1f, 2.5f},  {3.1f, 5.9f}};
        for (unsigned i = 0; i < std::size(clumps); ++i) {
            const auto p = clumps[i];
            const float rotation = i * 2.39996f;
            place(i % 3 == 0 ? PlantB : PlantA, p.x, p.y, {1.5f, 1.5f, 1.5f}, rotation);
            place(Grass, p.x + 0.65f, p.y - 0.35f, {1.5f, 1.5f, 1.5f}, rotation + 1);
            place(Grass, p.x - 0.45f, p.y + 0.4f, {1.0f, 1.2f, 1.0f}, rotation);
        }
        for (unsigned i = 0; i < 18; ++i) {
            const float a = i * 2.39996f, radius = 0.5f + (i % 5) * 0.35f;
            const float x = (i < 9 ? -4.8f : 4.7f) + std::cos(a) * radius;
            const float z = (i < 9 ? 3.6f : -3.1f) + std::sin(a) * radius;
            const float s = 0.24f + (i % 4) * 0.13f;
            place(Rock, x, z, {s, s * 0.6f, s}, a, 0.04f);
        }
        hasShadows = !contact.GetIndices().empty();
        if (hasShadows) {
            shadows.Init(contact);
        }
    }

    void Draw() {
        auto context = Renderer::GetDeviceContext();
        ComPtr<ID3D11RasterizerState> oldRaster;
        ComPtr<ID3D11DepthStencilState> oldDepth;
        ComPtr<ID3D11BlendState> oldBlend;
        UINT stencil = 0, mask = 0;
        float blend[4]{};
        context->RSGetState(oldRaster.GetAddressOf());
        context->OMGetDepthStencilState(oldDepth.GetAddressOf(), &stencil);
        context->OMGetBlendState(oldBlend.GetAddressOf(), blend, &mask);
        context->RSSetState(twoSided.Get());
        Renderer::SetBlendState(BS_NONE);
        context->OMSetDepthStencilState(noDepth.Get(), 0);
        Matrix identity = Matrix::Identity;
        Renderer::SetWorldMatrix(&identity);
        Renderer::SetViewMatrix(&identity);
        Renderer::SetProjectionMatrix(&identity);
        Renderer::SetUV(0, 0, 1, 1);
        backdropShader.SetGPU();
        white.SetGPU();
        backdrop.Draw();
        const auto size = ImGui::GetIO().DisplaySize;
        const float aspect = size.x / (std::max)(size.y, 1.0f);
        Matrix view, projection;
        enemyCamera.Matrices(aspect, view, projection);
        TitleRobotFigure::FirePose pose;
        if (m_startElapsed) {
            const float elapsed = *m_startElapsed;
            pose.shotSeconds = (std::max)(0.0f, elapsed - StartShotSeconds);
            pose.sensor =
                SmoothProgress(elapsed, 0.12f) * (1.0f - 0.45f * SmoothProgress(pose.shotSeconds, 0.28f));
            pose.recoil = 0.55f * SmoothProgress(pose.shotSeconds, 0.035f) *
                          (1.0f - SmoothProgress(pose.shotSeconds - 0.035f, 0.19f));
            // 正射影のまま押した角度を保ち、発砲に合わせて機体へ少し寄る。
            const float push = SmoothProgress(pose.shotSeconds, 0.38f);
            projection._11 *= 1.0f + 0.18f * push;
            projection._22 *= 1.0f + 0.18f * push;
            view._42 -= 0.65f * push;
        }
        const Matrix rotation = Matrix::CreateRotationY(yaw);
        const Matrix robotWorld = robotLocal * rotation;
        const bool showRobot =
            !enemyCamera.Occludes(Vector3::Transform({0, 2.5f, 0}, robotWorld), 5.4f * RobotDisplayScale);
        auto showProp = [&](const Prop& prop) {
            return !enemyCamera.Occludes(Vector3::Transform(prop.center, rotation), prop.radius);
        };
        // 背景と同じ頂点シェーダーで影の深度だけを描き、撮影カメラに戻す。
        castShadows.Capture([&] {
            if (showRobot) {
                robot.DrawBodyDepth(robotWorld, pose.recoil);
            }
            enemies.Draw(rotation, enemyCamera, selected, true);
            for (const auto& prop : props) {
                if ((prop.asset != Rock && prop.asset != Metal && prop.asset != Branch) || !showProp(prop)) {
                    continue;
                }
                auto world = prop.local * rotation;
                Renderer::SetWorldMatrix(&world);
                assets[prop.asset].mesh.renderer.Draw();
            }
        });
        Renderer::SetViewMatrix(&view);
        Renderer::SetProjectionMatrix(&projection);
        Renderer::SetDepthEnable(true);
        surface.SetGPU();
        auto draw = [&](unsigned id, Matrix world) {
            auto& asset = assets[id];
            if (!asset.ready) {
                return;
            }
            Renderer::SetWorldMatrix(&world);
            asset.mesh.renderer.BeforeDraw();
            for (const auto& part : asset.mesh.parts) {
                if (part.material < asset.textures.size() && asset.textures[part.material]) {
                    asset.textures[part.material]->SetGPU();
                } else {
                    white.SetGPU();
                }
                asset.mesh.renderer.DrawSubset(part.count, part.first, 0);
            }
        };
        Matrix standWorld = rotation;
        Renderer::SetWorldMatrix(&standWorld);
        white.SetGPU();
        plinth.Draw();
        groundShader.SetGPU();
        castShadows.BindGround();
        auto gravelView = gravel.GetShaderResourceView();
        context->PSSetShaderResources(1, 1, &gravelView);
        draw(Island, rotation);
        castShadows.UnbindGround();
        ID3D11ShaderResourceView* unbound = nullptr;
        context->PSSetShaderResources(1, 1, &unbound);
        if (hasShadows && showRobot) {
            Renderer::SetBlendState(BS_ALPHABLEND);
            Renderer::SetDepthEnable(false);
            shadowShader.SetGPU();
            white.SetGPU();
            Matrix world = rotation;
            Renderer::SetWorldMatrix(&world);
            shadows.Draw();
        }
        Renderer::SetDepthEnable(true);
        Renderer::SetBlendState(BS_NONE);
        surface.SetGPU();
        for (const auto& prop : props) {
            if (showProp(prop)) {
                draw(prop.asset, prop.local * rotation);
            }
        }
        enemies.Draw(rotation, enemyCamera, selected);
        if (showRobot) {
            robot.Draw(robotWorld, pose);
        }
        context->RSSetState(oldRaster.Get());
        context->OMSetDepthStencilState(oldDepth.Get(), stencil);
        context->OMSetBlendState(oldBlend.Get(), blend, mask);
    }
};

TitleScene::TitleScene() : m_diorama(std::make_unique<Diorama>()) {}

TitleScene::~TitleScene() = default;

void TitleScene::Update() {
    const float dt = GameTime::DeltaSeconds();
    auto& elapsed = m_diorama->m_startElapsed;
    if (elapsed) {
        const float previous = *elapsed;
        *elapsed = (std::min)(previous + dt, StartDuration);
        if (previous < StartShotSeconds && *elapsed >= StartShotSeconds) {
            GameSettings::PlayStartShotSound();
        }
        // 次のフレームでシーンが切り替わるまで、最後の真っ黒な画面を表示する。
        if (previous < StartDuration && *elapsed >= StartDuration) {
            Game::GetInstance()->ChangeScene(STAGE1);
        }
    } else if (!m_diorama->m_dragging && !m_diorama->IsInspectingEnemy()) {
        float RotationSpeed = 3.0f;
        const float step = dt * DirectX::XM_2PI / 180.0f * RotationSpeed;
        m_diorama->yaw = std::fmod(m_diorama->yaw + step, DirectX::XM_2PI);
    }
    m_diorama->UpdateCamera(dt);
}

void TitleScene::Draw(Camera*) {
    m_diorama->Draw();
}

void TitleScene::DrawUI() {
    const auto size = ImGui::GetIO().DisplaySize;
    if (m_diorama->m_startElapsed) {
        const float fade =
            SmoothProgress(*m_diorama->m_startElapsed - StartFadeSeconds, StartDuration - StartFadeSeconds);
        const int alpha = static_cast<int>(255.0f * fade + 0.5f);
        ImGui::GetForegroundDrawList()->AddRectFilled({0, 0}, size, IM_COL32(0, 0, 0, alpha));
        return;
    }
    const float width = std::clamp(size.x - 16.0f, 1.0f, 388.0f);
    const float gap = 6.0f;
    const float startWidth = (std::min)(236.0f, width);
    const float startHeight = std::clamp(size.y * 0.05f, 31.0f, 46.0f);
    const float secondaryHeight = std::clamp(size.y * 0.04f, 28.0f, 36.0f);
    const float bottomMargin = std::clamp(size.y * 0.018f, 4.0f, 16.0f);
    ImGui::SetNextWindowPos(ImVec2(size.x * 0.5f, size.y - bottomMargin), 0, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(width, startHeight + gap + secondaryHeight));
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, gap));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.16f, 0.17f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.33f, 0.33f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.40f, 0.39f, 1));
    ImGui::Begin("TitleScreen", nullptr, flags);
    if (m_inspectedEnemy) {
        ImGui::PopStyleVar(3);
        DrawEnemyInspector();
        ImGui::End();
        ImGui::PopStyleColor(3);
        return;
    }
    ImGui::SetCursorPosX((width - startWidth) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.32f, 0.32f, 1));
    if (ImGui::Button(Text(u8"ゲーム開始"), ImVec2(startWidth, startHeight))) {
        m_diorama->m_startElapsed = 0.0f;
        m_diorama->m_dragging = false;
        GameSettings::PlayStartSound();
    }
    ImGui::PopStyleColor();

    const ImVec2 smallButton((width - 2 * gap) / 3, secondaryHeight);
    auto pageButton = [&](const char8_t* label, Diorama::MenuPage page) {
        if (ImGui::Button(Text(label), smallButton)) {
            m_diorama->m_page = page;
            GameSettings::PlayMenuSound();
            ImGui::OpenPopup("##TitlePanel");
        }
    };
    pageButton(u8"設定", Diorama::MenuPage::Settings);
    ImGui::SameLine();
    pageButton(u8"ゲームルール", Diorama::MenuPage::Rules);
    ImGui::SameLine();
    if (ImGui::Button(Text(u8"ゲーム終了"), smallButton)) {
        GameSettings::PlayMenuSound();
        PostMessageW(Application::GetWindow(), WM_CLOSE, 0, 0);
    }
    ImGui::PopStyleVar(3);

    if (ImGui::IsPopupOpen("##TitlePanel")) {
        m_diorama->m_dragging = false;
        DrawMenuPanel();
    } else if (!m_diorama->IsInspectingEnemy()) {
        UpdateTurntableDrag();
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

void TitleScene::DrawMenuPanel() {
    const auto display = ImGui::GetIO().DisplaySize;
    const bool settings = m_diorama->m_page == Diorama::MenuPage::Settings;
    const float width = std::clamp(display.x - 24.0f, 1.0f, settings ? 540.0f : 1000.0f);
    const float height = std::clamp(display.y - 24.0f, 1.0f, settings ? 408.0f : 530.0f);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.075f, 0.10f, 0.11f, 1));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.015f, 0.023f, 0.025f, 0.70f));
    constexpr auto flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopupModal("##TitlePanel", nullptr, flags)) {
        ImGui::SetWindowFontScale(settings ? 1.08f : 1.18f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.47f, 1), "%s", Text(settings ? u8"設定" : u8"ゲームルール"));
        const float backWidth = 80.0f;
        ImGui::SameLine((std::max)(ImGui::GetCursorPosX(), width - 18 - backWidth));
        const bool back = ImGui::Button(Text(u8"戻る"), ImVec2(backWidth, 0)) ||
                          ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        if (back) {
            GameSettings::PlayMenuSound();
            ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();
        ImGui::Spacing();
        if (settings) {
            if (ImGui::BeginChild("SettingsBody")) {
                GameSettings::DrawControls();
            }
            ImGui::EndChild();
        } else if (const auto enemy = m_diorama->m_rules.Draw(); enemy && !back) {
            m_inspectedEnemy = enemy;
            m_diorama->FocusEnemy(*enemy);
            GameSettings::PlayMenuSound();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

namespace {
    // ボタンの全体をクリックできるようにし、矢印はフォントに依存せず描く。
    bool EnemyArrow(const char* id, ImGuiDir direction, const ImVec2& position, const ImVec2& size) {
        ImGui::SetCursorPos(position);
        const bool pressed = ImGui::Button(id, size);
        const auto start = ImGui::GetItemRectMin();
        const float cx = start.x + size.x * 0.5f;
        const float cy = start.y + size.y * 0.5f;
        const float radius = (std::min)(size.x, size.y) * 0.18f;
        const float side = direction == ImGuiDir_Left ? -1.0f : 1.0f;
        const ImVec2 points[] = {
            {cx - side * radius * 0.52f, cy - radius},
            {cx + side * radius * 0.52f, cy},
            {cx - side * radius * 0.52f, cy + radius},
        };
        ImGui::GetWindowDrawList()->AddPolyline(points, 3, IM_COL32(250, 218, 160, 255), 0, 2.6f);
        ImGui::SetItemTooltip("%s", Text(direction == ImGuiDir_Left ? u8"前の敵" : u8"次の敵"));
        return pressed;
    }
} // namespace

void TitleScene::DrawEnemyInspector() {
    const auto size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6);
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0));
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    if (ImGui::BeginPopupModal("##TitlePanel", nullptr, flags)) {
        ImGui::SetWindowFontScale(1.0f);
        const float margin = std::clamp(size.x * 0.016f, 4.0f, 24.0f);
        const float arrowWidth = std::clamp(size.x * 0.055f, 28.0f, 52.0f);
        const ImVec2 arrowSize(arrowWidth, arrowWidth * 1.18f);
        const float arrowY = (size.y - arrowSize.y) * 0.5f;
        const bool previous = EnemyArrow("##PreviousEnemy", ImGuiDir_Left, {margin, arrowY}, arrowSize);
        const bool next =
            EnemyArrow("##NextEnemy", ImGuiDir_Right, {size.x - margin - arrowWidth, arrowY}, arrowSize);
        const int direction = static_cast<int>(next || ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) -
                              static_cast<int>(previous || ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false));
        if (direction) {
            constexpr int count = static_cast<int>(TitleEnemyKind::Count);
            const int index = (static_cast<int>(*m_inspectedEnemy) + direction + count) % count;
            m_inspectedEnemy = static_cast<TitleEnemyKind>(index);
            m_diorama->FocusEnemy(*m_inspectedEnemy);
            GameSettings::PlayMenuSound();
        }

        const auto& enemy = GetTitleEnemyInfo(*m_inspectedEnemy);
        const float left = size.x * 0.52f;
        const float right = size.x - margin * 2 - arrowWidth;
        const float width = std::clamp(right - left, 1.0f, 450.0f);
        const float height = std::clamp(size.y - margin * 2, 1.0f, 332.0f);
        const float inset = std::clamp(width * 0.06f, 7.0f, 24.0f);
        ImGui::SetCursorPos(ImVec2((left + right - width) * 0.5f, (size.y - height) * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(inset, inset));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.078f, 0.09f, 0.97f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.26f, 0.34f, 0.35f, 1));
        bool back = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
        if (ImGui::BeginChild("EnemyDescription", {width, height}, ImGuiChildFlags_Border,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::SetWindowFontScale(1.70f);
            ImGui::TextUnformatted(Text(enemy.name));
            ImGui::SetWindowFontScale(1.15f);
            ImGui::Separator();
            ImGui::Spacing();
            const float buttonHeight = ImGui::GetFrameHeight() * 1.26f;
            ImGui::PushID(static_cast<int>(*m_inspectedEnemy));
            if (ImGui::BeginChild("EnemyText", {0, -buttonHeight - ImGui::GetStyle().ItemSpacing.y})) {
                ImGui::SetWindowFontScale(1.08f);
                const ImVec4 gold(0.96f, 0.78f, 0.48f, 1);
                ImGui::TextColored(gold, "%s", Text(u8"行動の特徴"));
                const float bodyWidth = (std::max)(1.0f, ImGui::GetContentRegionAvail().x);
                const float behaviorY = ImGui::GetCursorPosY();
                ImGui::TextWrapped("%s", Text(enemy.behavior));
                // 敵を切り替えても「対処のコツ」の見出しが上下しないよう、同じ高さを確保する。
                float behaviorHeight = 0;
                for (int i = 0; i < static_cast<int>(TitleEnemyKind::Count); ++i) {
                    const auto& info = GetTitleEnemyInfo(static_cast<TitleEnemyKind>(i));
                    behaviorHeight =
                        (std::max)(behaviorHeight,
                                   ImGui::CalcTextSize(Text(info.behavior), nullptr, false, bodyWidth).y);
                }
                ImGui::SetCursorPosY(behaviorY + behaviorHeight + ImGui::GetTextLineHeight() * 0.5f);
                ImGui::TextColored(gold, "%s", Text(u8"対処のコツ"));
                ImGui::TextWrapped("%s", Text(enemy.tip));
            }
            ImGui::EndChild();
            ImGui::PopID();
            if (ImGui::Button(Text(u8"一覧に戻る"), ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight))) {
                back = true;
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        if (back) {
            m_inspectedEnemy.reset();
            m_diorama->LeaveEnemyView();
            GameSettings::PlayMenuSound();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void TitleScene::UpdateTurntableDrag() {
    const auto& io = ImGui::GetIO();
    const auto size = io.DisplaySize;
    // ImGuiの入力が確定した描画フレームで、横の移動量を一度だけ角度に変換する。
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_diorama->m_dragging = !io.WantCaptureMouse && !ImGui::IsItemActive() &&
                                ImGui::IsMouseHoveringRect(ImVec2(0, 0), size, false);
    } else if (m_diorama->m_dragging && size.x > 0 && size.y > 0) {
        // 手前の縁がカーソルに追従する。左ドラッグは上から見て時計回り。
        constexpr float rimRadius = 10.0f;
        const float radiusPixels = rimRadius * size.y / ViewHeight(size.x / size.y);
        m_diorama->yaw = std::fmod(m_diorama->yaw - io.MouseDelta.x / radiusPixels, DirectX::XM_2PI);
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_diorama->m_dragging = false;
    }
    if (m_diorama->m_dragging) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
}
