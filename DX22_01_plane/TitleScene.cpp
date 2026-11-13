/// @brief 回転する小さな戦場。キャラクターを置く前の展示用ステージ。
#include "TitleScene.h"
#include "Game.h"
#include "GameTime.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
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
        void Backdrop() {
            m_vertices = {{{-1, -1, 0.999f}, {}, {1, 1, 1, 1}, {0, 1}},
                          {{-1, 1, 0.999f}, {}, {1, 1, 1, 1}, {0, 0}},
                          {{1, 1, 0.999f}, {}, {1, 1, 1, 1}, {1, 0}},
                          {{1, -1, 0.999f}, {}, {1, 1, 1, 1}, {1, 1}}};
            m_indices = {0, 1, 2, 0, 2, 3};
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
    Shader surface, backdropShader, shadowShader;
    Texture white;
    MeshRenderer backdrop, shadows;
    bool hasShadows = false;
    ComPtr<ID3D11RasterizerState> twoSided;
    ComPtr<ID3D11DepthStencilState> noDepth;
    float yaw = 0.4f;

    Diorama() {
        white.Load("assets/texture/white.png");
        surface.Create("shader/titleDioramaVS.hlsl", "shader/titleDioramaPS.hlsl");
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
        auto place = [&](unsigned asset, float x, float z, Vector3 scale, float rotation,
                         float sink = 0.02f) {
            const float y = assets[Island].mesh.Height(x, z);
            if (y < -1.0f || !assets[asset].ready) {
                return;
            }
            props.push_back({asset, Matrix::CreateScale(scale) * Matrix::CreateRotationY(rotation) *
                                        Matrix::CreateTranslation(x, y - sink, z)});
            if (asset == Rock || asset == Metal || asset == Branch) {
                contact.ContactShadow(assets[Island].mesh, x, z, scale.x * 0.52f);
            }
        };
        // 中央に対峙スペースを残し、外周に大小の岩をまとめる。
        place(Rock, -5.9f, 3.8f, {2.9f, 1.8f, 2.6f}, 0.2f, 0.24f);
        place(Rock, -4.3f, 5.5f, {2.1f, 1.3f, 1.8f}, 1.7f, 0.17f);
        place(Rock, -6.6f, 1.9f, {1.5f, 0.9f, 1.5f}, 2.4f, 0.12f);
        place(Rock, 5.8f, -3.9f, {2.3f, 1.3f, 2.1f}, 1.2f, 0.18f);
        place(Rock, 6.6f, -1.8f, {1.5f, 0.9f, 1.3f}, 2.8f, 0.12f);
        place(Rock, 3.8f, -5.7f, {1.1f, 0.7f, 1.2f}, 0.6f, 0.09f);
        place(Rock, 0.9f, 6.7f, {1.0f, 0.65f, 1.2f}, 1.3f, 0.08f);
        place(Metal, 4.2f, 3.6f, {4.1f, 4.1f, 4.1f}, -0.65f, 0.01f);
        place(Metal, 5.2f, 4.6f, {2.6f, 2.6f, 2.6f}, 0.65f, -0.06f);
        place(Branch, -4.4f, -4.8f, {3.4f, 3.4f, 3.4f}, 0.65f);
        const Vector2 clumps[] = {{-6, 4.9f},     {-4.2f, 4.3f},  {-6.7f, 0.2f}, {-5.8f, -3.4f},
                                  {-3.0f, -5.9f}, {-0.5f, -6.9f}, {2.0f, -6.1f}, {5.0f, -4.8f},
                                  {6.5f, -0.3f},  {6.1f, 2.5f},   {3.1f, 5.9f},  {-1.7f, 6.3f}};
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
        const float height = (std::max)(25.5f, 27.5f / (std::max)(aspect, 0.1f));
        Matrix view = DirectX::XMMatrixLookAtLH(Vector3(24, 25, -34), Vector3(0, -1.2f, 0), Vector3::Up);
        Matrix projection = DirectX::XMMatrixOrthographicLH(height * aspect, height, 0.1f, 150.0f);
        Renderer::SetViewMatrix(&view);
        Renderer::SetProjectionMatrix(&projection);
        Renderer::SetDepthEnable(true);
        surface.SetGPU();
        const Matrix rotation = Matrix::CreateRotationY(yaw);
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
        draw(Island, rotation);
        if (hasShadows) {
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
            draw(prop.asset, prop.local * rotation);
        }
        context->RSSetState(oldRaster.Get());
        context->OMSetDepthStencilState(oldDepth.Get(), stencil);
        context->OMSetBlendState(oldBlend.Get(), blend, mask);
    }
};

TitleScene::TitleScene() : m_diorama(std::make_unique<Diorama>()) {}

TitleScene::~TitleScene() = default;

void TitleScene::Update() {
    // 1周180秒。FPSによらず台座と全小物を一緒に回す。
    m_diorama->yaw =
        std::fmod(m_diorama->yaw + GameTime::DeltaSeconds() * DirectX::XM_2PI / 180.0f, DirectX::XM_2PI);
}

void TitleScene::Draw(Camera*) {
    m_diorama->Draw();
}

void TitleScene::DrawUI() {
    const auto size = ImGui::GetIO().DisplaySize;
    const float width = (std::max)(1.0f, (std::min)(260.0f, size.x - 32.0f));
    ImGui::SetNextWindowPos(ImVec2((size.x - width) * 0.5f, size.y * 0.87f));
    ImGui::SetNextWindowSize(ImVec2(width, 64));
    constexpr auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.22f, 0.23f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.34f, 0.33f, 1));
    ImGui::Begin("TitleScreen", nullptr, flags);
    if (ImGui::Button(reinterpret_cast<const char*>(u8"ゲーム開始"), ImVec2(width, 54))) {
        Game::GetInstance()->ChangeScene(STAGE1);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
