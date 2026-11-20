/// @brief 森の移動境界、ボス戦との交差領域、実素材の向きと材質色を検証する。
#include "../DX22_01_plane/ForestLayout.h"
#include "../DX22_01_plane/BossBattleRules.h"
#include "../DX22_01_plane/BossCatalog.h"
#include "../DX22_01_plane/FbxImportSettings.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using DirectX::SimpleMath::Vector3;

namespace {
    bool InsideForest(const Vector3& position, float radius) {
        return position.x >= ForestLayout::MinX + radius - 0.001f &&
               position.x <= ForestLayout::MaxX - radius + 0.001f &&
               position.z >= ForestLayout::MinZ + radius - 0.001f &&
               position.z <= ForestLayout::MaxZ - radius + 0.001f;
    }

    void CheckForestBounds() {
        for (float radius : {0.0f, 10.0f, 18.0f, 32.0f, 35.0f, 36.0f, 40.0f, 55.0f}) {
            for (float height : {-5.0f, 5.0f, 65.0f, 200.0f}) {
                const Vector3 interior(20, height, -300);
                assert(ForestLayout::Constrain(interior, radius) == interior);
                const auto low = ForestLayout::Constrain(Vector3(-4000, height, -4000), radius);
                const auto high = ForestLayout::Constrain(Vector3(4000, height, 4000), radius);
                assert(low == Vector3(ForestLayout::MinX + radius, height, ForestLayout::MinZ + radius));
                assert(high == Vector3(ForestLayout::MaxX - radius, height, ForestLayout::MaxZ - radius));
                // 壁にぶつかっても、ジャンプの高さと壁に沿う移動量は変えない。
                assert(ForestLayout::Constrain(Vector3(4000, height, -300), radius) ==
                       Vector3(ForestLayout::MaxX - radius, height, -300));
                assert(ForestLayout::Constrain(Vector3(20, height, -4000), radius) ==
                       Vector3(20, height, ForestLayout::MinZ + radius));
                for (const auto& position : {low, high, interior}) {
                    assert(InsideForest(position, radius));
                    assert(ForestLayout::Constrain(position, radius) == position);
                }
            }
        }
    }

    void CheckBossBoundaries() {
        std::array<float, BossTypeCount + 2> radii{10.0f, 55.0f};
        for (int index = 0; index < BossTypeCount; ++index) {
            radii[index + 2] = BossRadius(static_cast<BossKind>(index));
        }
        // 円の中心を最大の攻撃半径55だけ森の内側へ置く。各種類・角でも共通の通行域を保つ。
        for (float centerX : {ForestLayout::MinX + 10, 0.0f, ForestLayout::MaxX - 10}) {
            for (float centerZ : {ForestLayout::MinZ + 10, 0.0f, ForestLayout::MaxZ - 10}) {
                const Vector3 player(centerX, 5, centerZ);
                const auto center = ForestLayout::Constrain(player, 55);
                assert((center - player).Length() < 140.0f - 10.0f);
                for (float bodyRadius : radii) {
                    for (float arenaRadius : {140.0f, 500.0f, 800.0f}) {
                        for (int angleIndex = 0; angleIndex < 32; ++angleIndex) {
                            const float angle = DirectX::XM_2PI * angleIndex / 32;
                            const Vector3 outside =
                                center + Vector3(std::cos(angle) * 5000, 150, std::sin(angle) * 5000);
                            const auto forestPosition = ForestLayout::Constrain(outside, bodyRadius);
                            const auto resolved =
                                BossArena::ClampPosition(forestPosition, center, arenaRadius, bodyRadius);
                            assert(InsideForest(resolved, bodyRadius));
                            assert(resolved.y == outside.y);
                            Vector3 offset = resolved - center;
                            offset.y = 0;
                            assert(offset.Length() <= arenaRadius - bodyRadius + 0.002f);
                            const auto repeated =
                                BossArena::ClampPosition(ForestLayout::Constrain(resolved, bodyRadius),
                                                         center, arenaRadius, bodyRadius);
                            assert((repeated - resolved).Length() < 0.002f);
                        }
                    }
                }
            }
        }
    }

    void Require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void CheckPalette(const char* path) {
        std::ifstream file(path, std::ios::binary);
        std::array<unsigned char, 24> header{};
        file.read(reinterpret_cast<char*>(header.data()), header.size());
        Require(file.gcount() == static_cast<std::streamsize>(header.size()),
                std::string(path) + ": missing or truncated palette");
        constexpr std::array<unsigned char, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
        Require(std::equal(signature.begin(), signature.end(), header.begin()) && header[12] == 'I' &&
                    header[13] == 'H' && header[14] == 'D' && header[15] == 'R',
                std::string(path) + ": invalid PNG header");
        const auto dimension = [&](size_t offset) {
            return (static_cast<unsigned>(header[offset]) << 24) |
                   (static_cast<unsigned>(header[offset + 1]) << 16) |
                   (static_cast<unsigned>(header[offset + 2]) << 8) | header[offset + 3];
        };
        Require(dimension(16) > 0 && dimension(20) > 0, std::string(path) + ": empty palette");
    }

    void CheckArchOpening(const aiScene& scene, const aiVector3D& low, const aiVector3D& high) {
        const float scale = 456.4f / (high.y - low.y);
        const float centerX = (low.x + high.x) * 0.5f;
        Require((high.z - low.z) * scale <= ForestLayout::GateHalfDepth * 2,
                "Arch depth extends beyond the gate collider");
        // 足元と頭上の高さで石柱の内側を測り、通行判定の幅が実際の開口に収まることを確認する。
        for (float fraction : {0.10f, 0.25f}) {
            const float planeY = low.y + (high.y - low.y) * fraction;
            float halfOpening = 1e30f;
            for (unsigned meshIndex = 0; meshIndex < scene.mNumMeshes; ++meshIndex) {
                const auto& mesh = *scene.mMeshes[meshIndex];
                for (unsigned faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex) {
                    const auto& face = mesh.mFaces[faceIndex];
                    for (unsigned corner = 0; corner < 3; ++corner) {
                        const auto& from = mesh.mVertices[face.mIndices[corner]];
                        const auto& to = mesh.mVertices[face.mIndices[(corner + 1) % 3]];
                        if ((from.y - planeY) * (to.y - planeY) > 0 || std::abs(to.y - from.y) < 0.00001f) {
                            continue;
                        }
                        const float x = from.x + (to.x - from.x) * (planeY - from.y) / (to.y - from.y);
                        halfOpening = (std::min)(halfOpening, std::abs(x - centerX) * scale);
                    }
                }
            }
            Require(halfOpening >= ForestLayout::GateHalfWidth - 0.5f && halfOpening < 150.0f,
                    "Arch opening does not match the walkable passage");
        }
    }

    void CheckForestAsset(const char* name, const char* folder = "Ultimate Nature Pack by Quaternius/FBX/",
                          const aiVector3D& expectedSize = {}, const char* texturePath = nullptr) {
        const std::string path = std::string("DX22_01_plane/assets/model/Stage/") + folder + name + ".fbx";
        Assimp::Importer importer;
        ConfigureFbxImport(importer);
        const auto scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                                       aiProcess_PreTransformVertices | aiProcess_GenNormals);
        Require(scene && scene->HasMeshes(), path + ": " + importer.GetErrorString());
        aiVector3D low(1e30f), high(-1e30f);
        bool colored = false;
        bool hasTriangleArea = false;
        for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
            const auto& mesh = *scene->mMeshes[meshIndex];
            Require(mesh.mNumVertices > 0 && mesh.mNumFaces > 0 && mesh.HasNormals(),
                    path + ": empty mesh or missing normals");
            Require(mesh.mMaterialIndex < scene->mNumMaterials, path + ": invalid material index");
            const auto material = scene->mMaterials[mesh.mMaterialIndex];
            aiColor4D diffuse;
            Require(material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS,
                    path + ": missing material color");
            Require(std::isfinite(diffuse.r) && std::isfinite(diffuse.g) && std::isfinite(diffuse.b) &&
                        std::isfinite(diffuse.a) && diffuse.a > 0,
                    path + ": invalid material color");
            if (texturePath) {
                Require(mesh.HasTextureCoords(0), path + ": missing palette UV coordinates");
            } else {
                Require(material->GetTextureCount(aiTextureType_DIFFUSE) == 0,
                        path + ": new external texture dependency");
            }
            colored = colored || std::abs(diffuse.r - diffuse.g) > 0.001f ||
                      std::abs(diffuse.g - diffuse.b) > 0.001f;
            for (unsigned index = 0; index < mesh.mNumVertices; ++index) {
                const auto& position = mesh.mVertices[index];
                const auto& normal = mesh.mNormals[index];
                Require(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
                            std::isfinite(normal.x) && std::isfinite(normal.y) && std::isfinite(normal.z),
                        path + ": nonfinite vertex");
                low.x = (std::min)(low.x, position.x);
                low.y = (std::min)(low.y, position.y);
                low.z = (std::min)(low.z, position.z);
                high.x = (std::max)(high.x, position.x);
                high.y = (std::max)(high.y, position.y);
                high.z = (std::max)(high.z, position.z);
                if (texturePath) {
                    const auto& uv = mesh.mTextureCoords[0][index];
                    Require(std::isfinite(uv.x) && std::isfinite(uv.y) && uv.x >= 0 && uv.x <= 1 &&
                                uv.y >= 0 && uv.y <= 1,
                            path + ": invalid palette UV coordinates");
                }
                if (mesh.HasVertexColors(0)) {
                    const auto& color = mesh.mColors[0][index];
                    Require(std::isfinite(color.r) && std::isfinite(color.g) && std::isfinite(color.b) &&
                                std::isfinite(color.a),
                            path + ": nonfinite vertex color");
                }
            }
            for (unsigned index = 0; index < mesh.mNumFaces; ++index) {
                const auto& face = mesh.mFaces[index];
                Require(face.mNumIndices == 3, path + ": nontriangular face");
                for (unsigned corner = 0; corner < face.mNumIndices; ++corner) {
                    Require(face.mIndices[corner] < mesh.mNumVertices, path + ": invalid vertex index");
                }
                const auto edgeA = mesh.mVertices[face.mIndices[1]] - mesh.mVertices[face.mIndices[0]];
                const auto edgeB = mesh.mVertices[face.mIndices[2]] - mesh.mVertices[face.mIndices[0]];
                hasTriangleArea = hasTriangleArea || (edgeA ^ edgeB).SquareLength() > 1e-12f;
            }
        }
        Require(high.y - low.y > 0.0001f && high.x > low.x && high.z > low.z && hasTriangleArea,
                path + ": degenerate model bounds");
        Require(colored || texturePath, path + ": lost material colors");
        if (expectedSize.SquareLength() > 0) {
            Require((high - low - expectedSize).Length() < 0.05f,
                    path + ": asset dimensions or axes changed");
        }
        if (std::string(name) == "CommonTree_3") {
            // 実素材のY-up座標。高さの正規化前に確認し、Assimp更新で横倒しになる回帰を検出する。
            const aiVector3D expectedLow(-70.44f, -7.20f, -76.98f);
            const aiVector3D expectedHigh(57.06f, 338.43f, 79.19f);
            Require((low - expectedLow).Length() < 0.05f && (high - expectedHigh).Length() < 0.05f,
                    path + ": tree is no longer Y-up");
        }
        if (std::string(name) == "Arch") {
            CheckArchOpening(*scene, low, high);
        }
        if (std::string(name) == "Arch_bars") {
            const float scale = 386.0f / (high.y - low.y);
            Require((high.x - low.x) * scale >= ForestLayout::GateHalfWidth * 2,
                    "Closed bars do not cover the gate opening");
            Require((high.z - low.z) * scale * 0.5f + 50.0f <= ForestLayout::GateHalfDepth,
                    "Bars extend beyond the gate collider");
        }
        std::cout << name << " bounds " << low.x << ',' << low.y << ',' << low.z << " to " << high.x << ','
                  << high.y << ',' << high.z << '\n';
    }
} // namespace

int main() {
    try {
        CheckForestBounds();
        CheckBossBoundaries();
        for (const auto name :
             {"CommonTree_1", "CommonTree_3", "PineTree_1", "BirchTree_1", "Rock_Moss_2", "Rock_1", "Bush_1",
              "Grass_Short", "Flowers", "TreeStump_Moss", "WoodLog_Moss"}) {
            CheckForestAsset(name);
        }
        constexpr char palette[] = "DX22_01_plane/assets/model/Stage/lowpolynaturepacklite/colormap.png";
        CheckPalette(palette);
        constexpr char lite[] = "lowpolynaturepacklite/models/";
        CheckForestAsset("tent_blue", lite, {646.821f, 454.330f, 693.852f}, palette);
        CheckForestAsset("tent_red", lite, {646.821f, 454.330f, 693.852f}, palette);
        constexpr char dungeon[] = "FBX-20261116T081522Z-1-001/FBX/";
        CheckForestAsset("Arch", dungeon, {410.436f, 401.007f, 99.261f});
        CheckForestAsset("Arch_bars", dungeon, {247.826f, 338.815f, 3.033f});
        CheckForestAsset("Wall_Modular", dungeon, {200.074f, 200.539f, 43.807f});
        CheckForestAsset("Crate", dungeon, {137.977f, 130.763f, 131.089f});
        CheckForestAsset("Barrel", dungeon, {104.262f, 134.661f, 103.356f});
        CheckForestAsset("Woodfire", dungeon, {77.560f, 82.813f, 82.662f});
        CheckForestAsset("Column", dungeon, {130.412f, 406.506f, 130.412f});
        CheckForestAsset("Column2", dungeon, {130.412f, 407.225f, 130.412f});
        CheckForestAsset("Banner", dungeon, {236.102f, 379.856f, 21.930f});
        CheckForestAsset("Banner_wall", dungeon, {164.026f, 284.686f, 59.306f});
        CheckForestAsset("Pedestal2", dungeon, {174.303f, 231.467f, 174.303f});
        CheckForestAsset("Statue_Horse", dungeon, {174.303f, 585.481f, 337.836f});
        CheckForestAsset("WallCover_Modular", dungeon, {201.016f, 43.165f, 50.919f});
        CheckForestAsset("Torch", dungeon, {27.827f, 115.065f, 43.765f});
        std::cout << "Forest bounds, boss arenas, fortress assets, palette UVs and gate aperture passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
