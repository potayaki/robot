/// @file BoarAnimationRig.cpp
/// @brief Boarの骨格読込と接近・突進・死亡アニメーション。

#include "BoarAnimationRig.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace {
    constexpr double SourceFps = 24.0;
    constexpr double DeathEndFrame = 89.0; // 90から別の走行動作に戻るため、89で止める。

    aiQuaternion Slerp(const aiQuaternion& a, const aiQuaternion& b, float t) {
        aiQuaternion result;
        aiQuaternion::Interpolate(result, a, b, t);
        return result.Normalize();
    }

    template <class Keys, class Value, class Blend>
    Value Sample(const Keys& keys, double time, const Value& fallback, Blend blend) {
        if (keys.empty()) {
            return fallback;
        }
        if (time <= keys.front().mTime) {
            return keys.front().mValue;
        }
        if (time >= keys.back().mTime) {
            return keys.back().mValue;
        }
        const auto next = std::upper_bound(keys.begin(), keys.end(), time,
                                           [](double t, const auto& key) { return t < key.mTime; });
        const auto previous = next - 1;
        const float fraction = static_cast<float>((time - previous->mTime) / (next->mTime - previous->mTime));
        return blend(previous->mValue, next->mValue, fraction);
    }
} // namespace

struct BoarAnimationRig::Data {
    struct Node {
        /// 素材内で識別するための名前。
        std::string name;
        /// 親ノードの番号。-1は親を持たないルートを表す。
        int parent = -1;
        /// アニメーションを適用する前の基準姿勢。
        Pose bind;
    };

    struct Bone {
        /// 頂点または骨が対応する骨格ノードの番号。
        unsigned node;
        /// 基準姿勢からボーン空間へ変換する行列。
        aiMatrix4x4 offset;
    };

    struct Channel {
        /// このノードの位置アニメーションのキーフレーム一覧。
        std::vector<aiVectorKey> positions;
        /// 拡縮のアニメーションキー。
        std::vector<aiVectorKey> scales;
        /// 回転のアニメーションキー。
        std::vector<aiQuatKey> rotations;
    };

    /// 親子関係と基準姿勢を持つ骨格ノード一覧。
    std::vector<Node> nodes;
    /// 頂点の変形に使用する骨の情報。
    std::vector<Bone> bones;
    /// 骨ごとの位置・回転・拡縮のアニメーションキー。
    std::vector<Channel> channels;
    /// 描画に使用する頂点の配列。
    std::vector<Vertex> vertices;
    /// GLBに埋め込まれた圧縮画像のバイト列。
    std::vector<unsigned char> albedo;
    /// 素材のルート変換を打ち消すための逆行列。
    aiMatrix4x4 inverseRoot;
    /// アニメーションの1秒あたりの時間単位数。
    double ticksPerSecond = 1000.0;
};

bool BoarAnimationRig::Load(const std::string& file, std::string& error) {
    *this = BoarAnimationRig{};
    error.clear();
    // 同じBoarを増やすたびにGLBを読み直さず、骨格・頂点・内蔵画像を共有する。
    static std::weak_ptr<const Data> cached;
    static std::string cachedFile;
    if (file == cachedFile) {
        m_data = cached.lock();
    }
    try {
        if (!m_data) {
            auto data = std::make_shared<Data>();
            Assimp::Importer importer;
            const auto scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                                           aiProcess_LimitBoneWeights);
            if (!scene || !scene->mRootNode) {
                throw std::runtime_error(importer.GetErrorString());
            }
            if (scene->mNumMeshes != 1 || scene->mNumAnimations != 1) {
                throw std::runtime_error("Expected the supplied Boar mesh and Take 001 animation");
            }
            const auto mesh = scene->mMeshes[0];
            if (!mesh->HasNormals() || !mesh->HasTextureCoords(0) || !mesh->HasBones()) {
                throw std::runtime_error("Boar needs normals, UVs and skin weights");
            }
            data->inverseRoot = scene->mRootNode->mTransformation;
            data->inverseRoot.Inverse();

            std::unordered_map<std::string, unsigned> nodeIds;
            const std::function<void(aiNode*, int)> addNode = [&](aiNode* node, int parent) {
                Data::Node entry;
                entry.name = node->mName.C_Str();
                entry.parent = parent;
                node->mTransformation.Decompose(entry.bind.scale, entry.bind.rotation, entry.bind.position);
                const auto id = static_cast<unsigned>(data->nodes.size());
                nodeIds[entry.name] = id;
                data->nodes.push_back(entry);
                for (unsigned i = 0; i < node->mNumChildren; ++i) {
                    addNode(node->mChildren[i], static_cast<int>(id));
                }
            };
            addNode(scene->mRootNode, -1);

            std::vector<Vertex> vertices(mesh->mNumVertices);
            for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
                vertices[i].position = mesh->mVertices[i];
                vertices[i].normal = mesh->mNormals[i];
                vertices[i].uv = mesh->mTextureCoords[0][i];
            }
            for (unsigned b = 0; b < mesh->mNumBones; ++b) {
                const auto bone = mesh->mBones[b];
                data->bones.push_back({nodeIds.at(bone->mName.C_Str()), bone->mOffsetMatrix});
                for (unsigned w = 0; w < bone->mNumWeights; ++w) {
                    const auto& weight = bone->mWeights[w];
                    auto& vertex = vertices.at(weight.mVertexId);
                    auto slot = std::find(vertex.weights.begin(), vertex.weights.end(), 0.0f);
                    if (slot == vertex.weights.end()) {
                        throw std::runtime_error("More than four Boar skin weights");
                    }
                    vertex.bones[slot - vertex.weights.begin()] = b;
                    *slot = weight.mWeight;
                }
            }
            for (auto& vertex : vertices) {
                float total = 0;
                for (float weight : vertex.weights) {
                    total += weight;
                }
                if (total <= 0) {
                    throw std::runtime_error("Boar vertex has no skin weights");
                }
                for (float& weight : vertex.weights) {
                    weight /= total;
                }
            }
            for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
                const auto& face = mesh->mFaces[f];
                if (face.mNumIndices != 3) {
                    throw std::runtime_error("Boar has a non-triangle face");
                }
                for (unsigned k = 0; k < 3; ++k) {
                    data->vertices.push_back(vertices[face.mIndices[k]]);
                }
            }

            const auto animation = scene->mAnimations[0];
            data->ticksPerSecond = animation->mTicksPerSecond > 0 ? animation->mTicksPerSecond : SourceFps;
            if (animation->mDuration / data->ticksPerSecond < 119.0 / SourceFps - 0.01) {
                throw std::runtime_error("Boar's animation timeline is incomplete");
            }
            data->channels.resize(data->nodes.size());
            for (unsigned c = 0; c < animation->mNumChannels; ++c) {
                const auto source = animation->mChannels[c];
                auto& channel = data->channels[nodeIds.at(source->mNodeName.C_Str())];
                channel.positions.assign(source->mPositionKeys,
                                         source->mPositionKeys + source->mNumPositionKeys);
                channel.rotations.assign(source->mRotationKeys,
                                         source->mRotationKeys + source->mNumRotationKeys);
                channel.scales.assign(source->mScalingKeys, source->mScalingKeys + source->mNumScalingKeys);
            }

            // GLBにPNGが内蔵されている。FBX変換で付いたimage0.pngという外部参照に頼らない。
            aiString texturePath;
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
            const auto texture = scene->GetEmbeddedTexture(texturePath.C_Str());
            if (!texture || texture->mHeight != 0 || texture->mWidth == 0) {
                throw std::runtime_error("Boar embedded image is missing");
            }
            const auto bytes = reinterpret_cast<const unsigned char*>(texture->pcData);
            data->albedo.assign(bytes, bytes + texture->mWidth);
            m_data = data;
            cached = data;
            cachedFile = file;
        }

        for (const auto& node : m_data->nodes) {
            m_pose.push_back(node.bind);
        }
        m_blendFrom = m_pose;
        m_matrices.resize(m_data->bones.size());
        Evaluate();
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        *this = BoarAnimationRig{};
        return false;
    }
}

void BoarAnimationRig::Update(BoarMotion motion, float deltaSeconds, float progress) {
    if (!m_data || !std::isfinite(deltaSeconds) || deltaSeconds < 0) {
        return;
    }
    if (motion != m_motion) {
        m_motion = motion;
        m_seconds = 0;
        m_blendSeconds = 0;
        m_blendFrom = m_pose;
    }
    m_seconds += deltaSeconds;
    m_blendSeconds = (std::min)(m_blendSeconds + deltaSeconds, 0.12f);
    const float t = std::clamp(progress, 0.0f, 1.0f);

    // 元データは24fps、0～119フレーム。攻撃は準備→飛び出し→戻りに分け、
    // ゲームの予備動作・突進距離・硬直と同じ進行割合で再生する。
    switch (motion) {
        case BoarMotion::Idle:
            m_frame = std::fmod(m_seconds * SourceFps, 30.0);
            break;
        case BoarMotion::Run:
            m_frame = 90.0 + std::fmod(m_seconds * SourceFps, 29.0);
            break;
        case BoarMotion::Windup:
            m_frame = 30.0 + 14.0 * t;
            break;
        case BoarMotion::Charge:
            m_frame = 44.0 + 6.0 * t;
            break;
        case BoarMotion::Recover:
            m_frame = 50.0 + 9.0 * t;
            break;
        case BoarMotion::Death:
            m_frame = (std::min)(60.0 + m_seconds * SourceFps, DeathEndFrame);
            break;
    }
    Evaluate();
}

void BoarAnimationRig::Evaluate() {
    const double time = m_frame / SourceFps * m_data->ticksPerSecond;
    const float blend = m_blendSeconds / 0.12f;
    const auto lerp = [](const aiVector3D& a, const aiVector3D& b, float t) { return a + (b - a) * t; };
    std::vector<aiMatrix4x4> globals(m_data->nodes.size());
    for (size_t i = 0; i < m_data->nodes.size(); ++i) {
        const auto& node = m_data->nodes[i];
        const auto& channel = m_data->channels[i];
        Pose target;
        target.position = Sample(channel.positions, time, node.bind.position, lerp);
        target.scale = Sample(channel.scales, time, node.bind.scale, lerp);
        target.rotation = Sample(channel.rotations, time, node.bind.rotation, Slerp);
        if (node.name == "Hips" && m_motion != BoarMotion::Death) {
            // 移動は敵AIが担当する。素材内の前後移動を重ねて、判定や予測線とずれないようにする。
            target.position.x = node.bind.position.x;
            target.position.z = node.bind.position.z;
        }
        auto& pose = m_pose[i];
        pose.position = lerp(m_blendFrom[i].position, target.position, blend);
        pose.scale = lerp(m_blendFrom[i].scale, target.scale, blend);
        pose.rotation = Slerp(m_blendFrom[i].rotation, target.rotation, blend);
        aiMatrix4x4 local(pose.scale, pose.rotation, pose.position);
        globals[i] = node.parent < 0 ? local : globals[node.parent] * local;
    }
    for (size_t b = 0; b < m_data->bones.size(); ++b) {
        const auto& bone = m_data->bones[b];
        m_matrices[b] = m_data->inverseRoot * globals[bone.node] * bone.offset;
    }
}

bool BoarAnimationRig::DeathFinished() const {
    return m_motion == BoarMotion::Death && m_frame >= DeathEndFrame;
}

BoarAnimationRig::Bounds BoarAnimationRig::GetBounds() const {
    Bounds bounds{aiVector3D(1e20f, 1e20f, 1e20f), aiVector3D(-1e20f, -1e20f, -1e20f)};
    if (!m_data) {
        return {};
    }
    for (const auto& vertex : m_data->vertices) {
        aiVector3D position;
        for (unsigned w = 0; w < 4; ++w) {
            position += (m_matrices[vertex.bones[w]] * vertex.position) * vertex.weights[w];
        }
        for (unsigned axis = 0; axis < 3; ++axis) {
            bounds.min[axis] = (std::min)(bounds.min[axis], position[axis]);
            bounds.max[axis] = (std::max)(bounds.max[axis], position[axis]);
        }
    }
    return bounds;
}

const std::vector<BoarAnimationRig::Vertex>& BoarAnimationRig::GetVertices() const {
    return m_data->vertices;
}

const std::vector<unsigned char>& BoarAnimationRig::GetAlbedo() const {
    return m_data->albedo;
}
