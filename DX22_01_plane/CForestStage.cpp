/// @brief 木立と苔岩の外周、曲がる土の道、スライム用の開けた草地。
#include "CForestStage.h"
#include <array>
#include <cmath>
#include <random>
#include <string>

using namespace DirectX::SimpleMath;

namespace {
    enum Model {
        Oak,
        TallOak,
        Pine,
        Birch,
        MossRock,
        Rock,
        Bush,
        Grass,
        Flowers,
        Stump,
        Log,
        TentBlue,
        TentRed,
        Crate,
        Barrel,
        Campfire,
        Arch,
        Wall,
        Column,
        OrnateColumn,
        Banner,
        WallBanner,
        Pedestal,
        HorseStatue,
        WallCap,
        Torch
    };

    constexpr char AssetDirectory[] = "assets/model/Stage/Ultimate Nature Pack by Quaternius/FBX/";
    constexpr float GroundHalfSize = 1800.0f;
    constexpr float GroundStartZ = -1800.0f;
    constexpr float GroundEndZ = 6600.0f;
    constexpr float GroundCell = 40.0f;
    constexpr float StoneFloorStartZ = 2600.0f;
    constexpr float StoneFloorEndZ = 6000.0f;
    constexpr float StoneFloorHalfWidth = 800.0f;
    constexpr float GateHeight = 386.0f;
    constexpr float GateLift = 420.0f;
} // namespace

void CForestStage::Init() {
    const char* files[] = {"CommonTree_1", "CommonTree_3",   "PineTree_1",  "BirchTree_1",
                           "Rock_Moss_2",  "Rock_1",         "Bush_1",      "Grass_Short",
                           "Flowers",      "TreeStump_Moss", "WoodLog_Moss"};
    for (const auto file : files) {
        auto model = std::make_unique<ForestModel>();
        const auto path = std::string(AssetDirectory) + file + ".fbx";
        if (!model->Load(path.c_str())) {
            model.reset();
        }
        m_models.push_back(std::move(model));
    }

    const auto loadCampModel = [&](const std::string& path, const char* texture = nullptr) {
        auto model = std::make_unique<ForestModel>();
        if (!model->Load(path.c_str(), texture)) {
            model.reset();
        }
        m_models.push_back(std::move(model));
    };
    const std::string lite = "assets/model/Stage/lowpolynaturepacklite/";
    const std::string palette = lite + "colormap.png";
    loadCampModel(lite + "models/tent_blue.fbx", palette.c_str());
    loadCampModel(lite + "models/tent_red.fbx", palette.c_str());
    const std::string dungeon = "assets/model/Stage/FBX-20261116T081522Z-1-001/FBX/";
    for (const auto name :
         {"Crate", "Barrel", "Woodfire", "Arch", "Wall_Modular", "Column", "Column2", "Banner", "Banner_wall",
          "Pedestal2", "Statue_Horse", "WallCover_Modular", "Torch"}) {
        loadCampModel(dungeon + name + ".fbx");
    }
    m_gateModel = std::make_unique<ForestModel>();
    if (!m_gateModel->Load((dungeon + "Arch_bars.fbx").c_str())) {
        m_gateModel.reset();
    }

    // 敵の抽選とは独立した乱数で、再挑戦しても同じ地形を使う。
    std::mt19937 random(1701);
    const auto range = [&](float low, float high) {
        return std::uniform_real_distribution<float>(low, high)(random);
    };
    const auto place = [&](size_t model, float x, float z, float height) {
        m_props.push_back({model, Vector3(x, ForestLayout::GroundY, z), range(0, DirectX::XM_2PI), height});
    };
    const auto nearTent = [](float z) { return (z > 1280 && z < 1870) || (z > 1980 && z < 2470); };

    // 幹と大岩を移動範囲の外に置き、直進追跡するスライムにも通り道を残す。
    for (int layer = 0; layer < 3; ++layer) {
        const float offset = 110.0f + layer * 155.0f;
        for (float z = -1190; z <= ForestLayout::CampMaxZ + 180; z += 130) {
            if (layer < 2 && nearTent(z)) {
                continue;
            }
            for (float side : {-1.0f, 1.0f}) {
                place(static_cast<size_t>(random() % 4), side * (760 + offset + range(-25, 25)),
                      z + range(-40, 40), range(220, 370) + layer * 30);
            }
        }
        for (float x = -730; x <= 730; x += 135) {
            place(static_cast<size_t>(random() % 4), x + range(-35, 35), -1050 - offset,
                  range(220, 340) + layer * 35);
        }
    }

    // 苔の石積みと低木が、木の幹より手前にある移動境界を見せる。
    for (float z = -1040; z <= ForestLayout::CampMaxZ; z += 68) {
        if (nearTent(z)) {
            continue;
        }
        for (float side : {-1.0f, 1.0f}) {
            place(MossRock, side * range(830, 840), z + range(-10, 10), range(70, 105));
            place(Bush, side * range(825, 875), z + 30, range(45, 75));
        }
    }
    for (float x = -740; x <= 740; x += 68) {
        place(MossRock, x + range(-10, 10), -1120, range(70, 105));
    }

    // 入口と奥の目印。中央には大きな障害物を置かず、回避できる広場にする。
    place(Oak, -850, -790, 420);
    place(Oak, 850, -720, 405);
    place(TallOak, -875, 1080, 470);
    place(TallOak, 875, 1080, 450);
    place(Rock, -965, 320, 240);
    place(Rock, 1020, 470, 300);
    place(Stump, -850, -290, 65);
    place(Log, 885, -430, 60);

    const auto placeFacing = [&](size_t model, float x, float z, float height, float yaw,
                                 float elevation = 0) {
        m_props.push_back({model, Vector3(x, ForestLayout::GroundY + elevation, z), yaw, height});
    };
    for (size_t gate = 0; gate < ForestLayout::GateCount; ++gate) {
        const float z = ForestLayout::GatePositions[gate];
        placeFacing(Arch, 0, z, 456.4f, 0);
        // 開口は全門共通の260。城に入るほど周囲の壁を高くする。
        for (float side : {-1.0f, 1.0f}) {
            if (gate == 0) {
                for (float x : {335.0f, 540.0f, 745.0f}) {
                    placeFacing(Wall, side * x, z - 30, 215, 0);
                }
            } else {
                const float wallHeight = gate == 1 ? 300.0f : 430.0f;
                const float firstX = gate == 1 ? 370.0f : 440.0f;
                const float secondX = gate == 1 ? 650.0f : 820.0f;
                for (float x : {firstX, secondX}) {
                    placeFacing(Wall, side * x, z, wallHeight, 0);
                    placeFacing(WallCap, side * x, z, wallHeight * 0.215f, 0, wallHeight);
                }
                placeFacing(Banner, side * 350, z - 42, 265, 0);
                placeFacing(Torch, side * 190, z - 48, 95, 0, 140);
                placeFacing(OrnateColumn, side * 885, z, 520, 0);
            }
        }
    }

    // 中庭は低めの石壁と角柱、城内は高い壁と列柱で区別する。通行域は障害物で狭めない。
    for (float side : {-1.0f, 1.0f}) {
        for (float z = 2790; z <= 4190; z += 280) {
            placeFacing(Wall, side * 815, z, 300, DirectX::XM_PIDIV2);
            placeFacing(WallCap, side * 815, z, 64.5f, DirectX::XM_PIDIV2, 300);
        }
        for (float z : {2770.0f, 3340.0f, 3980.0f}) {
            placeFacing(Column, side * 885, z, 455, 0);
        }
        for (float z = 4460; z <= 5980; z += 380) {
            placeFacing(Wall, side * 830, z, 430, DirectX::XM_PIDIV2);
            placeFacing(WallCap, side * 830, z, 92.4f, DirectX::XM_PIDIV2, 430);
        }
        for (float z : {4480.0f, 5050.0f, 5620.0f}) {
            placeFacing(OrnateColumn, side * 885, z, 600, 0);
            placeFacing(Torch, side * 780, z, 100, side * DirectX::XM_PIDIV2, 150);
        }
        placeFacing(Crate, side * 830, 3000, 65, 0.2f);
        placeFacing(Barrel, side * 855, 3110, 78, 0);
        placeFacing(Crate, side * 835, 3870, 58, -0.12f);
        placeFacing(Banner, side * 805, 3500, 260, side * DirectX::XM_PIDIV2);
    }

    // 最奥は扉のない終端壁。壁の上に見える記念像を正面の目印にする。
    for (float x : {-630.0f, -210.0f, 210.0f, 630.0f}) {
        placeFacing(Wall, x, ForestLayout::KeepMaxZ + 60, 430, 0);
        placeFacing(WallCap, x, ForestLayout::KeepMaxZ + 60, 92.4f, 0, 430);
    }
    placeFacing(Pedestal, 0, ForestLayout::KeepMaxZ + 185, 220, 0);
    placeFacing(HorseStatue, 0, ForestLayout::KeepMaxZ + 185, 420, DirectX::XM_PI, 220);
    placeFacing(WallBanner, -310, ForestLayout::KeepMaxZ + 4, 245, 0, 120);
    placeFacing(WallBanner, 310, ForestLayout::KeepMaxZ + 4, 245, 0, 120);

    // テントと荷物は外周に寄せ、直進する敵が引っ掛からない戦闘広場を残す。
    placeFacing(TentBlue, -900, 1510, 160, DirectX::XM_PIDIV2);
    placeFacing(TentRed, 900, 1650, 150, -DirectX::XM_PIDIV2);
    placeFacing(TentBlue, -900, 2190, 140, DirectX::XM_PIDIV2);
    placeFacing(TentRed, 900, 2260, 165, -DirectX::XM_PIDIV2);
    placeFacing(Campfire, 185, 1840, 52, 0);
    placeFacing(Log, 185, 1940, 32, DirectX::XM_PIDIV2);
    placeFacing(Stump, 290, 1820, 30, 0);
    for (const auto& point :
         {Vector2(-815, 1290), Vector2(-815, 1990), Vector2(815, 2440), Vector2(815, 1890)}) {
        placeFacing(Crate, point.x, point.y, 48, 0.16f);
        placeFacing(Barrel, point.x + std::copysign(55.0f, point.x), point.y + 30, 53, 0);
    }

    // 道沿いの草花。戦闘広場では密度を下げ、敵の輪郭を読みやすくする。
    for (int i = 0; i < 360; ++i) {
        const float x = range(-730, 730);
        const float z = range(-1010, 865);
        const float pathDistance = std::abs(x - ForestLayout::PathCenter(z));
        const float clearingDistance = Vector2(x, z - 100).Length();
        if (pathDistance < 100 || (clearingDistance < 500 && i % 5 != 0)) {
            continue;
        }
        const bool flower = i % 7 == 0;
        place(flower ? Flowers : Grass, x, z, flower ? range(9, 16) : range(7, 17));
    }
    for (int i = 0; i < 210; ++i) {
        const float x = range(-730, 730);
        const float z = range(1060, ForestLayout::CampMaxZ - 40);
        if (std::abs(x - ForestLayout::PathCenter(z)) > 140 && std::abs(x) > 330) {
            place(i % 8 == 0 ? Flowers : Grass, x, z, range(8, 16));
        }
    }
    BuildGround();
}

void CForestStage::Update() {
    m_gate.Update(GameTime::DeltaSeconds());
}

void CForestStage::BuildGround() {
    std::vector<VERTEX_3D> vertices;
    std::vector<unsigned> indices;
    constexpr int columns = static_cast<int>(GroundHalfSize * 2 / GroundCell);
    constexpr int depthCells = static_cast<int>((GroundEndZ - GroundStartZ) / GroundCell);
    for (int row = 0; row <= depthCells; ++row) {
        const float z = GroundStartZ + row * GroundCell;
        for (int column = 0; column <= columns; ++column) {
            const float x = -GroundHalfSize + column * GroundCell;
            const float pathDistance = std::abs(x - ForestLayout::PathCenter(z));
            const float path = 1.0f - std::clamp((pathDistance - 65.0f) / 85.0f, 0.0f, 1.0f);
            const float clearing = 1.0f - std::clamp((Vector2(x, z - 100).Length() - 250) / 350, 0.0f, 1.0f);
            const float camp =
                1.0f - std::clamp((Vector2(x, (z - 1850) * 0.8f).Length() - 200) / 380, 0.0f, 1.0f);
            const float noise = std::sin(x * 0.039f + z * 0.021f) * std::cos(z * 0.053f) * 0.025f;
            const Color grass(0.23f + noise, 0.36f + noise, 0.12f + noise, 1);
            const Color earth(0.49f + noise, 0.37f + noise, 0.20f + noise, 1);
            VERTEX_3D vertex{};
            vertex.position = Vector3(x, ForestLayout::GroundY + 0.04f, z);
            vertex.normal = Vector3::UnitY;
            const float wear = (std::max)({path * 0.9f, clearing * 0.32f, camp * 0.78f});
            vertex.color = Color::Lerp(grass, earth, wear);
            vertices.push_back(vertex);
        }
    }
    for (int row = 0; row < depthCells; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float centerX = -GroundHalfSize + (column + 0.5f) * GroundCell;
            const float centerZ = GroundStartZ + (row + 0.5f) * GroundCell;
            // 石畳の下には別の面を重ねず、遠景の深度競合も防ぐ。
            if (std::abs(centerX) < StoneFloorHalfWidth && centerZ >= StoneFloorStartZ &&
                centerZ < StoneFloorEndZ) {
                continue;
            }
            const unsigned a = row * (columns + 1) + column;
            const unsigned b = a + columns + 1;
            indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }

    const auto quad = [&](float left, float right, float front, float back, const Color& color) {
        const unsigned first = static_cast<unsigned>(vertices.size());
        for (const auto& point :
             {Vector2(left, front), Vector2(left, back), Vector2(right, front), Vector2(right, back)}) {
            VERTEX_3D vertex{};
            vertex.position = Vector3(point.x, ForestLayout::GroundY + 0.04f, point.y);
            vertex.normal = Vector3::UnitY;
            vertex.color = color;
            vertices.push_back(vertex);
        }
        indices.insert(indices.end(), {first, first + 1, first + 2, first + 2, first + 1, first + 3});
    };
    // 石と目地を同一平面の隣接した四角形で作り、一度の描画にまとめる。
    for (int room = 0; room < 2; ++room) {
        const float front = room == 0 ? StoneFloorStartZ : ForestLayout::CourtyardMaxZ;
        const float back = room == 0 ? ForestLayout::CourtyardMaxZ : StoneFloorEndZ;
        const int rows = room == 0 ? 24 : 17;
        const int stoneColumns = room == 0 ? 20 : 16;
        const float width = StoneFloorHalfWidth * 2 / stoneColumns;
        const float depth = (back - front) / rows;
        const Color mortar = room == 0 ? Color(0.20f, 0.22f, 0.18f, 1) : Color(0.105f, 0.115f, 0.14f, 1);
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < stoneColumns; ++column) {
                const float x = -StoneFloorHalfWidth + column * width;
                const float z = front + row * depth;
                const float shade = static_cast<float>((row * 7 + column * 11) % 9) * 0.009f;
                const bool central = std::abs(x + width * 0.5f) < 110;
                Color stone = room == 0 ? Color(0.37f + shade, 0.36f + shade, 0.31f + shade, 1)
                                        : Color(0.23f + shade, 0.25f + shade, 0.29f + shade, 1);
                if (room == 1 && central) {
                    stone = Color(0.30f + shade, 0.20f + shade * 0.5f, 0.19f + shade * 0.5f, 1);
                }
                constexpr float joint = 2.0f;
                quad(x + joint, x + width - joint, z + joint, z + depth - joint, stone);
                quad(x, x + width, z, z + joint, mortar);
                quad(x, x + width, z + depth - joint, z + depth, mortar);
                quad(x, x + joint, z + joint, z + depth - joint, mortar);
                quad(x + width - joint, x + width, z + joint, z + depth - joint, mortar);
            }
        }
    }
    m_groundVertices.Create(vertices);
    m_groundIndices.Create(indices);
    m_groundIndexCount = static_cast<unsigned>(indices.size());
    m_shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    material.TextureEnable = FALSE;
    m_groundMaterial.Create(material);
}

void CForestStage::Draw(Camera* camera) {
    camera->SetCamera();
    Matrix world = Matrix::Identity;
    Renderer::SetWorldMatrix(&world);
    m_shader.SetGPU();
    m_groundMaterial.SetGPU();
    m_groundVertices.SetGPU();
    m_groundIndices.SetGPU();
    auto context = Renderer::GetDeviceContext();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_groundIndexCount, 0, 0);
    const Matrix view = camera->GetViewMatrix();
    for (const auto& prop : m_props) {
        // ワールド原点からの距離を使わず、最奥の部屋でも現在のカメラ位置から判定する。
        const Vector3 center = Vector3::Transform(prop.feet + Vector3(0, prop.height * 0.5f, 0), view);
        const float radius = prop.height * 2.5f; // 横長の笠石と倒木も含む保守的な境界。
        const float drawDistance = prop.model == Grass || prop.model == Flowers ? 900.0f : 2600.0f;
        if (center.z + radius < 0 ||
            center.LengthSquared() > (drawDistance + radius) * (drawDistance + radius)) {
            continue;
        }
        if (m_models[prop.model]) {
            m_models[prop.model]->Draw(camera, prop.feet, prop.yaw, prop.height);
        }
    }
    if (m_gateModel) {
        for (size_t gate = 0; gate < ForestLayout::GateCount; ++gate) {
            const float opening = m_gate.OpenAmount(gate);
            const float lift = opening * opening * (3.0f - 2.0f * opening) * GateLift;
            const Vector3 feet(0, ForestLayout::GroundY + lift, ForestLayout::GatePositions[gate] - 50);
            const Vector3 center = Vector3::Transform(feet + Vector3(0, GateHeight * 0.5f, 0), view);
            constexpr float radius = GateHeight * 2.5f;
            constexpr float drawDistance = 2600.0f;
            // 遠くの城壁を省いたとき、格子だけが宙に残らないよう同じ距離で省く。
            if (center.z + radius >= 0 &&
                center.LengthSquared() <= (drawDistance + radius) * (drawDistance + radius)) {
                m_gateModel->Draw(camera, feet, 0, GateHeight);
            }
        }
    }
}

void CForestStage::Uninit() {
    m_props.clear();
    m_models.clear();
    m_gateModel.reset();
    m_gate = ForestGateProgress{};
    m_groundVertices = VertexBuffer<VERTEX_3D>{};
    m_groundIndices = IndexBuffer{};
    m_groundIndexCount = 0;
}
