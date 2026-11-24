/// @brief 木立と苔岩の外周、曲がる土の道、スライム用の開けた草地。
#include "CForestStage.h"
#include "ForestMeadow.h"
#include "AimCursor.h"
#include "Application.h"
#include "Game.h"
#include "CPlayer.h"
#include "shader/meadowViewDistance.hlsli"
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
        Torch,
        TallGrass,
        GroundLeaves,
        PinkFlowers,
        YellowFlowers,
        OrangeFlowers
    };

    constexpr char AssetDirectory[] = "assets/model/Stage/Ultimate Nature Pack by Quaternius/FBX/";
    constexpr float GroundHalfSize = ForestLayout::TerrainMaxX;
    constexpr float GroundStartZ = ForestLayout::TerrainMinZ;
    constexpr float GroundEndZ = 6600.0f;
    constexpr float GroundCell = ForestLayout::TerrainCell;
    constexpr float GroundTextureSize = 190.0f;
    constexpr unsigned WindBufferSlot = 10; // meadowWind.hlsli の b10。
    constexpr double WindPeriodSeconds = 64.0;
    constexpr float StoneFloorStartZ = ForestLayout::CampMaxZ;
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
        const bool windAffected = m_models.size() == Grass || m_models.size() == Flowers;
        if (!model->Load(path.c_str(), nullptr, windAffected)) {
            model.reset();
        }
        m_models.push_back(std::move(model));
    }

    const auto loadCampModel = [&](const std::string& path, const char* texture = nullptr,
                                   bool windAffected = false) {
        auto model = std::make_unique<ForestModel>();
        if (!model->Load(path.c_str(), texture, windAffected)) {
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
    loadCampModel(lite + "models/grass03.fbx", palette.c_str(), true);
    loadCampModel(std::string(AssetDirectory) + "Plant_1.fbx", nullptr, true);
    for (const auto name : {"flower02_pink", "flower02_yellow", "flower02_orange"}) {
        loadCampModel(lite + "models/" + name + ".fbx", palette.c_str(), true);
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
    const auto place = [&](size_t model, float x, float z, float height, float buried = 0) {
        const float feetY = ForestLayout::GroundHeight(x, z) - buried;
        m_props.push_back({model, Vector3(x, feetY, z), range(0, DirectX::XM_2PI), height});
    };

    // 道の曲がりごとに別の樹種が見えるよう、土手の上に大小の木立をまとめる。
    struct Grove {
        float x, z, spreadX, spreadZ, height;
        Model tree;
        int count;
    };

    constexpr Grove groves[] = {
        {-520, -1390, 420, 180, 315, Oak, 16},   {370, -1480, 510, 240, 360, Pine, 20},
        {-1020, -760, 155, 350, 255, Birch, 17}, {-1110, -100, 270, 300, 365, Oak, 19},
        {-1030, 680, 200, 345, 295, Birch, 14},  {1050, -710, 185, 390, 285, Oak, 16},
        {1180, 235, 305, 490, 410, Pine, 24},    {1035, 1140, 205, 315, 320, TallOak, 14},
        {-1370, 1490, 120, 440, 315, Pine, 12},  {-1035, 2190, 170, 325, 335, TallOak, 14},
        {1380, 2150, 150, 465, 290, Birch, 15}};
    for (const auto& grove : groves) {
        for (int i = 0; i < grove.count; ++i) {
            const float angle = range(0, DirectX::XM_2PI);
            const float radius = std::sqrt(range(0, 1));
            const float x = grove.x + std::cos(angle) * radius * grove.spreadX;
            const float z = grove.z + std::sin(angle) * radius * grove.spreadZ;
            const auto tree = i % 4 == 0 ? static_cast<Model>(random() % 4) : grove.tree;
            place(tree, x, z, grove.height * range(0.65f, 1.18f), 12);
        }
    }

    // 岩の露頭とその足元の灌木で外周を示す。切れ目から奥の林も見える。
    constexpr Vector2 outcrops[] = {{-870, -860}, {-930, -325},  {-905, 145}, {-860, 710}, {-880, 1100},
                                    {-900, 2170}, {880, -610},   {955, -50},  {1030, 430}, {885, 890},
                                    {910, 1660},  {-600, -1180}, {250, -1190}};
    for (const auto& center : outcrops) {
        place(MossRock, center.x, center.y, range(70, 110), 18);
        for (int i = 0; i < 7; ++i) {
            const float x = center.x + range(-42, 42);
            const float z = center.y + range(-110, 110);
            place(i % 3 == 0 ? Rock : Bush, x, z, range(32, 76), 11);
        }
    }


    place(TallOak, -850, 155, 475, 10);
    place(Rock, 1090, 420, 245, 65);
    place(Pine, 1165, 515, 480, 16);
    place(Stump, -815, -365, 60);
    place(Log, 865, -480, 45);
    // 道が土手を回る場所に白樺と苔岩を寄せ、広場へ入る手前の目印にする。
    place(Birch, -825, -380, 295, 7);
    place(MossRock, -860, -465, 112, 25);
    place(Bush, -805, -420, 53, 8);

    // 道端の平石は頭だけを土から出し、ロボットが歩いて越せる高さにする。
    constexpr Vector2 trailStones[] = {{-396, -745}, {-371, -730}, {-422, -706}, {-609, -523},
                                       {-628, -494}, {-370, -177}, {-342, -152}};
    for (const auto& stone : trailStones) {
        const float height = range(21, 34);
        place(MossRock, stone.x, stone.y, height, height - range(3, 7));
        place(Grass, stone.x + 12, stone.y - 9, range(4, 8));
    }

    const auto placeFacing = [&](size_t model, float x, float z, float height, float yaw,
                                 float elevation = 0) {
        m_props.push_back({model, Vector3(x, ForestLayout::GroundHeight(x, z) + elevation, z), yaw, height});
    };
    const auto placeStoneWall = [&](float x, float z, float height, float yaw) {
        placeFacing(Wall, x, z, height, yaw);
        placeFacing(WallCap, x, z, height * 0.215f, yaw, height);
    };
    for (size_t gate = 0; gate < ForestLayout::GateCount; ++gate) {
        const float z = ForestLayout::GatePositions[gate];
        const float gateX = ForestLayout::GateCentersX[gate];
        placeFacing(Arch, gateX, z, 456.4f, 0);
        // アーチ外縁から左右の外周まで、各壁材の実寸に合わせて並べる。
        constexpr float archHalfWidth = 230.0f;
        constexpr float wallHeights[] = {210, 300, 430};
        for (float side : {-1.0f, 1.0f}) {
            const float edge = side < 0 ? ForestLayout::MinX - 80 : ForestLayout::MaxX + 80;
            for (float span = archHalfWidth; span < std::abs(edge - gateX);) {
                // 森側の古い石垣は門柱から離れるほど低くなり、背後の木立がのぞく。
                const float ruin = gate == 0 ? std::clamp((span - archHalfWidth) / 440, 0.0f, 1.0f) : 0;
                const float wallHeight = wallHeights[gate] - 108 * ruin;
                const float wallStep = wallHeight * 0.98f;
                const float x = gateX + side * (span + wallStep * 0.5f);
                if (gate == 0) {
                    placeFacing(Wall, x, z, wallHeight, 0);
                } else {
                    placeStoneWall(x, z, wallHeight, 0);
                }
                span += wallStep;
            }
            if (gate > 0) {
                placeFacing(Banner, gateX + side * 350, z - 50, 265, 0);
                placeFacing(Torch, gateX + side * 190, z - 48, 95, 0, 140);
                placeFacing(OrnateColumn, edge, z, 520, 0);
            }
        }
    }
    // 最初の石壁には木々がかぶさり、右手に開けた門の輪郭が残る。
    place(Oak, -570, ForestLayout::GateZ + 32, 330);
    place(TallOak, -410, ForestLayout::GateZ + 40, 405);
    place(Birch, -308, ForestLayout::GateZ + 28, 270);
    place(MossRock, -460, ForestLayout::GateZ - 18, 63, 12);

    // 中庭の東は見張り塔、西は石壁越しに古木が見える庭にする。
    for (float side : {-1.0f, 1.0f}) {
        const float firstLowSegment = side < 0 ? 3070.0f : 3350.0f;
        for (float z = 2790; z <= 4190; z += 280) {
            if (z == firstLowSegment || z == firstLowSegment + 280) {
                // 塔と庭の足元まで見渡せるよう、同じ区間を四つの低い石積みでふさぐ。
                for (float offset : {-105.0f, -35.0f, 35.0f, 105.0f}) {
                    placeStoneWall(side * 815, z + offset, 75, DirectX::XM_PIDIV2);
                }
            } else {
                placeStoneWall(side * 815, z, 300, DirectX::XM_PIDIV2);
            }
        }
        for (float z = 4460; z <= 5980; z += 380) {
            placeStoneWall(side * 830, z, 430, DirectX::XM_PIDIV2);
        }
    }
    placeStoneWall(1095, 3260, 470, 0);
    placeStoneWall(860, 3495, 470, DirectX::XM_PIDIV2);
    placeStoneWall(1330, 3495, 470, DirectX::XM_PIDIV2);
    placeStoneWall(1095, 3730, 470, 0);
    placeFacing(WallBanner, 1095, 3210, 235, 0, 215);
    placeFacing(WallBanner, 807, 3460, 215, DirectX::XM_PIDIV2, 120);
    placeFacing(Torch, 802, 3340, 85, DirectX::XM_PIDIV2, 118);
    placeFacing(Column, -865, 2920, 425, 0);
    placeFacing(TallOak, -1090, 3100, 605, 0.64f);
    placeFacing(Birch, -980, 3290, 490, -0.32f);
    placeFacing(Bush, -920, 3090, 118, 0.72f);
    placeFacing(Bush, -936, 3210, 86, -0.27f);
    placeFacing(Column, -857, 3830, 455, 0);
    placeFacing(Column, -855, 4015, 392, 0);

    // 城内は右手の旗から左奥の列柱を経て、正面の像へ視線を導く。
    for (float z : {4520.0f, 4770.0f}) {
        placeFacing(OrnateColumn, 866, z, 610, 0);
        placeFacing(Torch, 780, z, 92, DirectX::XM_PIDIV2, 165);
    }
    placeFacing(WallBanner, 779, 4645, 325, DirectX::XM_PIDIV2, 95);
    for (float z : {5130.0f, 5350.0f, 5570.0f}) {
        placeFacing(OrnateColumn, -874, z, 655, 0);
    }
    placeFacing(WallBanner, -779, 5240, 265, -DirectX::XM_PIDIV2, 120);
    placeFacing(Torch, -780, 5460, 100, -DirectX::XM_PIDIV2, 155);

    // 終端の石壁は中央で低くなり、ロボットの目線からも馬像の全身が見える。
    for (float side : {-1.0f, 1.0f}) {
        float span = 0;
        for (float height : {75.0f, 125.0f, 200.0f, 250.0f, 300.0f}) {
            const float width = height * 0.99f;
            placeStoneWall(side * (span + width * 0.5f), ForestLayout::KeepMaxZ + height * 0.11f + 6, height,
                           0);
            span += width;
        }
        placeFacing(WallBanner, side * 300, ForestLayout::KeepMaxZ, 145, 0, 40);
    }
    placeFacing(Pedestal, 0, ForestLayout::KeepMaxZ + 110, 112, 0);
    placeFacing(HorseStatue, 0, ForestLayout::KeepMaxZ + 110, 194, DirectX::XM_PI + 0.95f, 112);

    // 西の宿営は焚き火を囲み、東の補給所では大きな赤い幕の足元に荷物を集める。
    placeFacing(TentBlue, -1010, 1390, 142, 0.92f);
    placeFacing(TentBlue, -985, 1685, 130, 2.04f);
    placeFacing(Campfire, -670, 1530, 43, 0.4f);
    placeFacing(Log, -806, 1575, 30, 1.16f);
    placeFacing(Stump, -782, 1460, 28, 0.57f);
    placeFacing(TentRed, 955, 2065, 168, -1.22f);
    placeFacing(Crate, 817, 2255, 51, -0.16f);
    placeFacing(Crate, 883, 2276, 44, 0.12f);
    placeFacing(Barrel, 849, 2326, 60, 0.42f);

    // 群落ごとに花の色をそろえ、道の肩から草地へ広がる形をつける。
    // 道の中央と焚き火、門の前には背の高い草花を置かない。
    struct FlowerPatch {
        float offsetFromPath, z, radiusX, radiusZ;
        Model bloom;
        int clumps;
    };

    constexpr FlowerPatch patches[] = {
        {-103, -830, 21, 52, YellowFlowers, 6}, {120, -744, 29, 76, PinkFlowers, 8},
        {-144, -574, 38, 71, Flowers, 5},       {303, -495, 66, 113, YellowFlowers, 7},
        {-188, -260, 52, 98, PinkFlowers, 6},   {124, -112, 32, 55, OrangeFlowers, 4},
        {-486, 90, 80, 138, YellowFlowers, 7},  {346, 270, 70, 104, Flowers, 6},
        {-167, 438, 40, 82, PinkFlowers, 5},    {208, 668, 44, 72, OrangeFlowers, 5},
        {-181, 1130, 34, 65, Flowers, 5},       {246, 1335, 58, 108, PinkFlowers, 6},
        {-536, 1470, 57, 99, YellowFlowers, 6}, {-191, 1800, 60, 88, OrangeFlowers, 5},
        {586, 2040, 80, 136, YellowFlowers, 7}, {-227, 2315, 45, 81, PinkFlowers, 5}};
    for (const auto& patch : patches) {
        for (int i = 0; i < patch.clumps; ++i) {
            const float angle = range(0, DirectX::XM_2PI);
            const float radius = std::sqrt(range(0, 1));
            const float z = patch.z + std::sin(angle) * radius * patch.radiusZ;
            const float x =
                ForestLayout::PathCenter(z) + patch.offsetFromPath + std::cos(angle) * radius * patch.radiusX;
            place(i % 3 == 0 ? GroundLeaves : Grass, x - 7, z + 4, range(5, 11));
            place(TallGrass, x + 9, z + range(-8, 12), range(12, 22));
            place(patch.bloom, x, z, range(13, 21));
            place(patch.bloom, x + range(-16, 16), z + range(-18, 18), range(9, 16));
        }
    }
    // 背の低い草は花畑の間にも広げ、踏み跡の縁から草地へ密度を変える。
    for (float z = -930; z < ForestLayout::CampMaxZ - 150; z += range(52, 100)) {
        if (std::abs(z - ForestLayout::GateZ) < 130) {
            continue;
        }
        const float side = random() % 2 ? -1.0f : 1.0f;
        const float margin = range(88, 260);
        const float spread = range(26, 68);
        for (int tuft = 0; tuft < 6; ++tuft) {
            const float tuftZ = z + range(-spread, spread);
            const float x = ForestLayout::PathCenter(tuftZ) + side * (margin + range(-22, 35));
            place(tuft % 3 == 0 ? GroundLeaves : TallGrass, x, tuftZ, range(5, 13));
        }
    }
    BuildGround();
    if (!m_sunShadows.Init()) {
        std::cerr << "Forest sun shadow map unavailable\n";
    }
    m_grass.Init();
    Renderer::CreateConstantBuffer(sizeof(Vector4), m_windBuffer.ReleaseAndGetAddressOf());
}

void CForestStage::Update() {
    const float seconds = GameTime::DeltaSeconds();
    m_gate.Update(seconds);
    m_windSeconds = std::fmod(m_windSeconds + seconds, WindPeriodSeconds);
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
            const float cover = ForestMeadow::Cover(x, z);
            const float clearing =
                1.0f - std::clamp((Vector2(x - 80, (z - 60) * 1.3f).Length() - 180) / 310, 0.0f, 1.0f);
            const float noise = std::sin(x * 0.039f + z * 0.021f) * std::cos(z * 0.053f) * 0.025f;
            const Color grass(0.42f + noise + clearing * 0.08f, 0.61f + noise + clearing * 0.065f,
                              0.285f + noise, 1);
            const Color earth(0.55f + noise, 0.39f + noise, 0.23f + noise, 1);
            VERTEX_3D vertex{};
            vertex.position = Vector3(x, ForestLayout::GroundHeight(x, z), z);
            vertex.uv = Vector2(x, z) / GroundTextureSize;
            vertex.normal = Vector3(ForestLayout::GroundHeight(x - GroundCell, z) -
                                        ForestLayout::GroundHeight(x + GroundCell, z),
                                    2 * GroundCell,
                                    ForestLayout::GroundHeight(x, z - GroundCell) -
                                        ForestLayout::GroundHeight(x, z + GroundCell));
            vertex.normal.Normalize();
            const float wear = z < StoneFloorStartZ ? (1 - cover) * 0.94f : 0;
            vertex.color = Color::Lerp(grass, earth, wear);
            vertex.color.w = cover;
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
            // 外縁は城壁に固定し、中央の敷石を前後の門へ続ける。
            const float weight = 1.0f - std::abs(point.x) / StoneFloorHalfWidth;
            const float x = point.x + ForestLayout::PathCenter(point.y) * weight;
            vertex.position = Vector3(x, ForestLayout::GroundY, point.y);
            vertex.uv = Vector2(x, point.y) / GroundTextureSize;
            vertex.normal = Vector3::UnitY;
            vertex.color = color;
            vertex.color.w = 0; // 石畳は風による色変化の対象外。
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
                if (central) {
                    stone = room == 0 ? Color(0.54f + shade, 0.48f + shade, 0.35f + shade, 1)
                                      : Color(0.38f + shade, 0.21f + shade * 0.5f, 0.18f + shade * 0.5f, 1);
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
    m_shader.Create("shader/groundSkyVS.hlsl", "shader/forestGroundPS.hlsl");
    MATERIAL material{};
    material.TextureEnable = m_groundDetail.Load("assets/model/cylinder/gravel.jpg");
    // 細粒テクスチャの平均輝度を補い、草と踏み跡の色を保つ。
    const float brightness = material.TextureEnable ? 1.18f : 1.0f;
    material.Diffuse = Color(brightness, brightness, brightness, 1);
    m_groundMaterial.Create(material);
}

void CForestStage::Draw(Camera* camera) {
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (!players.empty() && !players.front()->IsDead()) {
        CPlayer* player = players.front();
        m_sunShadows.Capture(player->GetPosition(), Renderer::GetLightDirection(),
                             [player] { player->DrawSunShadow(); });
        m_sunShadows.BindGround();
    }
    camera->SetCamera();
    Matrix world = Matrix::Identity;
    Renderer::SetWorldMatrix(&world);
    auto context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11Buffer> previousVS, previousPS;
    context->VSGetConstantBuffers(WindBufferSlot, 1, previousVS.GetAddressOf());
    context->PSGetConstantBuffers(WindBufferSlot, 1, previousPS.GetAddressOf());
    if (m_windBuffer) {
        const Vector4 wind(static_cast<float>(m_windSeconds), 0, 0, 0);
        context->UpdateSubresource(m_windBuffer.Get(), 0, nullptr, &wind, 0, 0);
    }
    context->VSSetConstantBuffers(WindBufferSlot, 1, m_windBuffer.GetAddressOf());
    context->PSSetConstantBuffers(WindBufferSlot, 1, m_windBuffer.GetAddressOf());
    m_shader.SetGPU();
    m_groundMaterial.SetGPU();
    m_groundDetail.SetGPU();
    m_groundVertices.SetGPU();
    m_groundIndices.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(m_groundIndexCount, 0, 0);
    m_grass.Draw(camera);
    m_sunShadows.UnbindGround();
    const Matrix view = camera->GetViewMatrix();
    const float tanHalfY = std::tan(AimCursor::FieldOfView * 0.5f);
    const float tanHalfX = tanHalfY * Application::GetWidth() / (std::max)(1u, Application::GetHeight());
    for (const auto& prop : m_props) {
        // ワールド原点からの距離を使わず、最奥の部屋でも現在のカメラ位置から判定する。
        const Vector3 center = Vector3::Transform(prop.feet + Vector3(0, prop.height * 0.5f, 0), view);
        const float radius = prop.height * 2.5f; // 横長の笠石と倒木も含む保守的な境界。
        const bool groundCover = prop.model == Grass || prop.model == Flowers || prop.model >= TallGrass;
        const float drawDistance = groundCover ? GroundCoverFadeEnd : 2600.0f;
        const float farZ = center.z + radius;
        if (farZ < 0 || std::abs(center.x) > farZ * tanHalfX + radius ||
            std::abs(center.y) > farZ * tanHalfY + radius ||
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
            const Vector3 feet(ForestLayout::GateCentersX[gate], ForestLayout::GroundY + lift,
                               ForestLayout::GatePositions[gate] - 50);
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
    context->VSSetConstantBuffers(WindBufferSlot, 1, previousVS.GetAddressOf());
    context->PSSetConstantBuffers(WindBufferSlot, 1, previousPS.GetAddressOf());
}

void CForestStage::Uninit() {
    m_props.clear();
    m_models.clear();
    m_gateModel.reset();
    m_gate = ForestGateProgress{};
    m_groundVertices = VertexBuffer<VERTEX_3D>{};
    m_groundIndices = IndexBuffer{};
    m_groundDetail = Texture{};
    m_grass = ForestGrass{};
    m_sunShadows = StageSunShadows{};
    m_windBuffer.Reset();
    m_windSeconds = 0;
    m_groundIndexCount = 0;
}
