/// @brief 実素材の上下・正面とタイトルの射撃姿勢がAssimpの版によって変わらないか検証する。
#include "../DX22_01_plane/FbxImportSettings.h"
#include "../DX22_01_plane/RobotAnimationRig.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/version.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
    void Require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void CheckAssetAxes(const char* path, const aiVector3D& expectedLow, const aiVector3D& expectedHigh) {
        Assimp::Importer importer;
        ConfigureFbxImport(importer);
        const auto scene = importer.ReadFile(std::string("DX22_01_plane/assets/model/") + path,
                                             aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                                 aiProcess_PreTransformVertices);
        Require(scene && scene->mNumMeshes > 0, std::string(path) + ": " + importer.GetErrorString());
        aiVector3D low(1e30f), high(-1e30f);
        for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
            const auto& mesh = *scene->mMeshes[i];
            for (unsigned v = 0; v < mesh.mNumVertices; ++v) {
                const auto& p = mesh.mVertices[v];
                Require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z), "Nonfinite vertex");
                low.x = (std::min)(low.x, p.x);
                low.y = (std::min)(low.y, p.y);
                low.z = (std::min)(low.z, p.z);
                high.x = (std::max)(high.x, p.x);
                high.y = (std::max)(high.y, p.y);
                high.z = (std::max)(high.z, p.z);
            }
        }
        // 5.2.5で確認した素材座標の範囲。Y/Zの交換や符号反転を、寸法正規化の前に検出する。
        Require((low - expectedLow).Length() < 0.02f && (high - expectedHigh).Length() < 0.02f,
                std::string(path) + ": FBX axes changed");
    }

    void CheckTitleRobotPose() {
        RobotAnimationRig rig;
        std::string error;
        Require(rig.Load("DX22_01_plane/assets/model/Player", error), "Robot: " + error);
        rig.SetLowerBodyScale(1.15f);
        constexpr float yaw = 0.62f;
        constexpr float pitch = -0.08f;
        rig.Update(RobotMotion::Forward, 0.20f, 1.0f, yaw, pitch);
        const aiVector3D expected(-std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                                  -std::cos(yaw) * std::cos(pitch));
        for (bool left : {true, false}) {
            const int node = rig.FindNode(left ? "ShotgunTop_L" : "ShotgunTop_R");
            Require(node >= 0, "Missing shotgun joint");
            const auto& matrix = rig.GetMatrices()[node];
            auto direction = aiMatrix3x3(matrix) * aiVector3D(0, 0, left ? 1.0f : -1.0f);
            direction.Normalize();
            Require((direction - expected).Length() < 0.001f, "Title robot gun points the wrong way");
            const auto position = matrix * aiVector3D();
            Require(position.y > 400.0f && position.y < 450.0f, "Title robot gun is not above its feet");
        }
    }
} // namespace

int main() {
    try {
        std::cout << "Assimp " << aiGetVersionMajor() << '.' << aiGetVersionMinor() << '.'
                  << aiGetVersionPatch() << '\n';
        CheckAssetAxes("drone/Drone.fbx", {-69.6871f, -60.6273f, -121.191f}, {69.6871f, 54.7711f, -11.2998f});
        CheckAssetAxes("slime/Slime.fbx", {-48.9559f, -53.1931f, -93.9551f}, {48.9559f, 48.956f, 0});
        CheckAssetAxes("mushroom/Mushroom.fbx", {-56.7732f, -56.7752f, -93.199f},
                       {56.7732f, 56.7752f, 0.00294f});
        CheckAssetAxes("Ghost/Ghost.fbx", {-98.7075f, 75.1593f, -73.6733f}, {100.661f, 363.761f, 131.318f});
        CheckAssetAxes("Player/Player.fbx", {-542.886f, 0, -487.77f}, {542.886f, 789.122f, 399.914f});
        CheckTitleRobotPose();
        std::cout << "FBX asset axes and title robot firing pose passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
