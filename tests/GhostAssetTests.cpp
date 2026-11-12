#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
    Assimp::Importer importer;
    auto scene = importer.ReadFile("DX22_01_plane/assets/model/Ghost/Ghost.fbx",
                                   aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
                                       aiProcess_PreTransformVertices);
    assert(scene && scene->mNumMeshes > 0);
    std::cout << "Meshes " << scene->mNumMeshes << " textures " << scene->mNumTextures << '\n';
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        auto mesh = scene->mMeshes[i];
        assert(mesh->mNumVertices > 0 && mesh->mNumFaces > 0);
        assert(mesh->mMaterialIndex < scene->mNumMaterials);
        aiVector3D low = mesh->mVertices[0], high = low;
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            auto p = mesh->mVertices[v];
            assert(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z));
            low.x = std::min(low.x, p.x);
            low.y = std::min(low.y, p.y);
            low.z = std::min(low.z, p.z);
            high.x = std::max(high.x, p.x);
            high.y = std::max(high.y, p.y);
            high.z = std::max(high.z, p.z);
        }
        std::cout << mesh->mName.C_Str() << " material " << mesh->mMaterialIndex << " bounds " << low.x << ','
                  << low.y << ',' << low.z << " to " << high.x << ',' << high.y << ',' << high.z << '\n';
    }
    for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
        aiString texture;
        aiColor4D color;
        assert(scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS);
        scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texture);
        assert(texture.length == 0); // 現在の素材は画像ではなく材質色で描く。
        std::cout << "Material " << i << " RGB " << color.r << ',' << color.g << ',' << color.b << '\n';
    }
}
