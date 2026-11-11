/// @file RobotAnimationRig.cpp
/// @brief ロボットの素材読込、歩行の補間、上半身の照準回転。

#include "RobotAnimationRig.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
    constexpr unsigned ImportFlags = aiProcess_Triangulate | aiProcess_ConvertToLeftHanded;
    constexpr float BlendSeconds = 0.15f;

    aiQuaternion BlendRotation(const aiQuaternion& a, const aiQuaternion& b, float amount) {
        aiQuaternion result;
        aiQuaternion::Interpolate(result, a, b, amount); // 最短経路のSlerp。
        return result.Normalize();
    }

    template <typename Keys, typename Value, typename Interpolate>
    Value Sample(const Keys& keys, double time, const Value& fallback, Interpolate interpolate) {
        if (keys.empty()) {
            return fallback;
        }
        if (time <= keys.front().time) {
            return keys.front().value;
        }
        if (time >= keys.back().time) {
            return keys.back().value;
        }
        const auto next = std::upper_bound(keys.begin(), keys.end(), time,
                                           [](double t, const auto& key) { return t < key.time; });
        const auto previous = next - 1;
        const float amount = static_cast<float>((time - previous->time) / (next->time - previous->time));
        return interpolate(previous->value, next->value, amount);
    }
} // namespace

RobotMotion SelectRobotWalkMotion(float moveX, float moveZ, float lowerBodyYaw) {
    if (moveX * moveX + moveZ * moveZ <= 0.000001f) {
        return RobotMotion::Idle;
    }

    const float forward = moveX * std::sin(lowerBodyYaw) + moveZ * std::cos(lowerBodyYaw);
    const float right = moveX * std::cos(lowerBodyYaw) - moveZ * std::sin(lowerBodyYaw);

    // カメラ基準のWASDではなく、実際の移動方向と脚の向きの関係で決める。
    if (std::abs(forward) >= std::abs(right)) {
        return forward > 0.0f ? RobotMotion::Forward : RobotMotion::Backward;
    }
    return right > 0.0f ? RobotMotion::Right : RobotMotion::Left;
}

int RobotAnimationRig::FindNode(const std::string& name) const {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool RobotAnimationRig::Load(const std::string& directory, std::string& error) {
    *this = RobotAnimationRig{};
    error.clear();
    try {
        Assimp::Importer importer;
        // モデルと別ファイルのアニメーションで同じ骨名・変換規則にする。
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        const auto scene = importer.ReadFile(directory + "/Player.fbx", ImportFlags);
        if (!scene || !scene->mRootNode) {
            throw std::runtime_error(importer.GetErrorString());
        }
        m_inverseRoot = scene->mRootNode->mTransformation;
        m_inverseRoot.Inverse();

        const std::function<void(aiNode*, int)> addNode = [&](aiNode* node, int parent) {
            Node entry;
            entry.name = node->mName.C_Str();
            entry.parent = parent;
            entry.upperBody = entry.name == "Top_M" || (parent >= 0 && m_nodes[parent].upperBody);
            node->mTransformation.Decompose(entry.bind.scale, entry.bind.rotation, entry.bind.position);
            const int index = static_cast<int>(m_nodes.size());
            m_nodes.push_back(entry);
            for (unsigned i = 0; i < node->mNumChildren; ++i) {
                addNode(node->mChildren[i], index);
            }
        };
        addNode(scene->mRootNode, -1);
        m_upperBodyNode = FindNode("Top_M");
        if (m_upperBodyNode < 0) {
            throw std::runtime_error("Robot upper-body joint Top_M is missing");
        }

        for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
            const auto mesh = scene->mMeshes[i];
            // FBXにはLOD0とLOD1の両方がある。プレイヤー1体分の詳細版だけを描く。
            if (std::string(mesh->mName.C_Str()).find("_LOD0") == std::string::npos) {
                continue;
            }
            if (!mesh->HasNormals() || !mesh->HasTextureCoords(0)) {
                throw std::runtime_error("Robot mesh needs normals and UVs");
            }

            std::vector<int> boneForVertex(mesh->mNumVertices, -1);
            Part part;
            std::vector<int> boneNodes(mesh->mNumBones);
            for (unsigned b = 0; b < mesh->mNumBones; ++b) {
                const auto bone = mesh->mBones[b];
                boneNodes[b] = FindNode(bone->mName.C_Str());
                if (boneNodes[b] < 0) {
                    throw std::runtime_error("Robot bone missing from hierarchy");
                }
                for (unsigned w = 0; w < bone->mNumWeights; ++w) {
                    const auto weight = bone->mWeights[w];
                    if (weight.mWeight < 0.0001f) {
                        continue;
                    }
                    if (std::abs(weight.mWeight - 1.0f) > 0.0001f || boneForVertex[weight.mVertexId] >= 0) {
                        throw std::runtime_error("Robot requires rigid, single-bone vertices");
                    }
                    boneForVertex[weight.mVertexId] = static_cast<int>(b);
                }
            }
            for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
                const auto& face = mesh->mFaces[f];
                if (face.mNumIndices != 3) {
                    throw std::runtime_error("Invalid robot face");
                }
                for (unsigned k = 0; k < 3; ++k) {
                    const unsigned v = face.mIndices[k];
                    const int b = boneForVertex[v];
                    if (b < 0) {
                        throw std::runtime_error("Robot vertex has no bone");
                    }
                    const auto& offset = mesh->mBones[b]->mOffsetMatrix;
                    aiMatrix3x3 normalMatrix(offset);
                    normalMatrix.Inverse().Transpose();
                    Vertex vertex;
                    vertex.node = static_cast<unsigned>(boneNodes[b]);
                    vertex.position = offset * mesh->mVertices[v];
                    vertex.normal = (normalMatrix * mesh->mNormals[v]).Normalize();
                    vertex.uv = mesh->mTextureCoords[0][v];
                    part.vertices.push_back(vertex);
                }
            }
            if (!part.vertices.empty()) {
                m_parts.push_back(std::move(part));
            }
        }
        if (m_parts.empty()) {
            throw std::runtime_error("Robot LOD0 geometry is missing");
        }

        const char* files[] = {"Idle", "WalkForward", "WalkBackward", "Walk_L", "Walk_R"};
        for (size_t i = 0; i < m_clips.size(); ++i) {
            Assimp::Importer animationImporter;
            animationImporter.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
            const std::string path = directory + "/Animations/A_ISO_Mech_" + files[i] + "_01.fbx";
            const auto animationScene = animationImporter.ReadFile(path, ImportFlags);
            if (!animationScene || animationScene->mNumAnimations == 0) {
                throw std::runtime_error("Cannot load robot animation: " + path);
            }
            const auto animation = animationScene->mAnimations[0];
            auto& clip = m_clips[i];
            clip.duration = (std::max)(1.0, animation->mDuration);
            clip.ticksPerSecond = animation->mTicksPerSecond > 0 ? animation->mTicksPerSecond : 30;
            clip.channels.resize(m_nodes.size());
            for (unsigned c = 0; c < animation->mNumChannels; ++c) {
                const auto source = animation->mChannels[c];
                const int node = FindNode(source->mNodeName.C_Str());
                if (node < 0) {
                    throw std::runtime_error("Animation bone does not match robot");
                }
                auto& channel = clip.channels[node];
                for (unsigned k = 0; k < source->mNumPositionKeys; ++k) {
                    channel.positions.push_back(
                        {source->mPositionKeys[k].mTime, source->mPositionKeys[k].mValue});
                }
                for (unsigned k = 0; k < source->mNumScalingKeys; ++k) {
                    channel.scales.push_back({source->mScalingKeys[k].mTime, source->mScalingKeys[k].mValue});
                }
                for (unsigned k = 0; k < source->mNumRotationKeys; ++k) {
                    channel.rotations.push_back(
                        {source->mRotationKeys[k].mTime, source->mRotationKeys[k].mValue});
                }
            }
        }
        for (const auto& node : m_nodes) {
            m_pose.push_back(node.bind);
        }
        m_blendFrom = m_pose;
        m_globals.resize(m_nodes.size());
        Evaluate();
        return true;
    } catch (const std::exception& failure) {
        error = failure.what();
        *this = RobotAnimationRig{};
        return false;
    }
}

void RobotAnimationRig::Update(RobotMotion motion, float deltaSeconds, float playbackRate, float upperBodyYaw,
                               float upperBodyPitch) {
    if (m_parts.empty() || deltaSeconds <= 0) {
        return;
    }
    m_upperBodyYaw = upperBodyYaw;
    m_upperBodyPitch = upperBodyPitch;
    if (motion != m_motion) {
        // 切り替え途中に別方向へ動いても、現在表示している姿勢から補間を始める。
        m_blendFrom = m_pose;
        m_motion = motion;
        m_time = 0;
        m_blendTime = 0;
    }
    const auto& clip = m_clips[static_cast<size_t>(m_motion)];
    m_time = std::fmod(m_time + deltaSeconds * (std::max)(0.0f, playbackRate) * clip.ticksPerSecond,
                       clip.duration);
    m_blendTime = (std::min)(BlendSeconds, m_blendTime + deltaSeconds);
    Evaluate();
}

void RobotAnimationRig::Evaluate() {
    const auto lerp = [](const aiVector3D& a, const aiVector3D& b, float t) { return a + (b - a) * t; };
    const float blend = m_blendTime / BlendSeconds;
    const auto& clip = m_clips[static_cast<size_t>(m_motion)];
    std::vector<aiMatrix4x4> hierarchy(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const auto& node = m_nodes[i];
        const auto& channel = clip.channels[i];
        Pose target;
        target.position = Sample(channel.positions, m_time, node.bind.position, lerp);
        target.scale = Sample(channel.scales, m_time, node.bind.scale, lerp);
        target.rotation = Sample(channel.rotations, m_time, node.bind.rotation, BlendRotation);
        if (node.name == "Root_M") {
            // 移動はCPlayerが担当する。素材側の前進量を重ねず、足だけを動かす。
            target.position.x = node.bind.position.x;
            target.position.z = node.bind.position.z;
        }
        auto& pose = m_pose[i];
        pose.position = lerp(m_blendFrom[i].position, target.position, blend);
        pose.scale = lerp(m_blendFrom[i].scale, target.scale, blend);
        pose.rotation = BlendRotation(m_blendFrom[i].rotation, target.rotation, blend);
        const aiMatrix4x4 local(pose.scale, pose.rotation, pose.position);
        hierarchy[i] = node.parent < 0 ? local : hierarchy[node.parent] * local;
        m_globals[i] = m_inverseRoot * hierarchy[i];
    }
    ApplyLowerBodyScale();
    ApplyUpperBodyAim();
}

void RobotAnimationRig::SetLowerBodyScale(float scale) {
    if (!std::isfinite(scale)) {
        return;
    }
    m_lowerBodyScale = std::clamp(scale, 1.0f, 1.6f);
    if (!m_globals.empty()) {
        Evaluate();
    }
}

void RobotAnimationRig::ApplyLowerBodyScale() {
    if (m_lowerBodyScale == 1.0f || m_upperBodyNode < 0) {
        return;
    }

    // 足元を基準にSK_Mech_Bottomの骨格とメッシュを一緒に拡大する。
    // 頂点だけ拡大すると、IKが使う脚の長さと見た目が一致しなくなる。
    aiMatrix4x4 lowerScale;
    aiMatrix4x4::Scaling(aiVector3D(m_lowerBodyScale), lowerScale);

    // 上半身は接続位置だけ追従させる。頭・胴・武器のサイズや照準回転は変えない。
    const auto pivot = m_globals[m_upperBodyNode] * aiVector3D();
    aiMatrix4x4 upperOffset;
    aiMatrix4x4::Translation(pivot * (m_lowerBodyScale - 1.0f), upperOffset);
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        m_globals[i] = (m_nodes[i].upperBody ? upperOffset : lowerScale) * m_globals[i];
    }
}

void RobotAnimationRig::ApplyUpperBodyAim() {
    if (m_upperBodyNode < 0 || (m_upperBodyYaw == 0.0f && m_upperBodyPitch == 0.0f)) {
        return;
    }

    // 骨のローカル軸は斜めを向いているため、モデル空間で照準の回転を作る。
    // 原点ではなくTop_Mの関節を中心に回し、胴体が脚から離れて公転するのを防ぐ。
    const aiVector3D pivot = m_globals[m_upperBodyNode] * aiVector3D(0, 0, 0);
    aiMatrix4x4 yaw, pitch;
    aiMatrix4x4::RotationY(m_upperBodyYaw, yaw);
    aiMatrix4x4::RotationX(m_upperBodyPitch, pitch);
    // 素材の正面は-Zなので、X軸の正回転が上向き。上下を付けてから左右へ旋回する。
    aiMatrix4x4 turn = yaw * pitch;
    const aiVector3D translation = pivot - turn * pivot;
    turn.a4 = translation.x;
    turn.b4 = translation.y;
    turn.c4 = translation.z;

    // アニメーション計算後の上半身だけに適用する。元の姿勢へ書き戻さないので、
    // フレームごとの回転が累積せず、脚の歩行や方向切り替えの補間にも混ざらない。
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].upperBody) {
            m_globals[i] = turn * m_globals[i];
        }
    }
}
