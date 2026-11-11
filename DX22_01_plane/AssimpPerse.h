/// @file AssimpPerse.h
/// @brief Assimpで読み込んだ素材をゲーム用の頂点・材質へ変換。

#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace AssimpPerse {
    struct VERTEX {
        /// メッシュ名
        std::string meshname;
        /// 位置
        aiVector3D pos;
        /// 法線
        aiVector3D normal;
        /// 頂点カラー
        aiColor4D color;
        /// テクスチャ座標
        aiVector3D texcoord;
        /// マテリアルインデックス
        int materialindex;
        /// マテリアル名
        std::string mtrlname;
    };

    struct SUBSET {
        /// メッシュ名
        std::string meshname;
        /// マテリアルインデックス
        int materialindex;
        /// 頂点バッファのベース
        unsigned int VertexBase;
        /// 頂点数
        unsigned int VertexNum;
        /// インデックスバッファのベース
        unsigned int IndexBase;
        /// インデックス数
        unsigned int IndexNum;
        /// マテリアル名
        std::string mtrlname;
    };

    struct MATERIAL {
        /// マテリアル名
        std::string mtrlname;
        /// アンビエント
        aiColor4D Ambient;
        /// ディフューズ
        aiColor4D Diffuse;
        /// スペキュラ
        aiColor4D Specular;
        /// エミッション
        aiColor4D Emission;
        /// シャイネス
        float Shiness;
        /// テクスチャ名
        std::string texturename;
    };

    void GetModelData(std::string filename, std::string texturedirectory);
    std::vector<SUBSET> GetSubsets();
    std::vector<std::vector<VERTEX>> GetVertices();
    std::vector<std::vector<unsigned int>> GetIndices();
    std::vector<MATERIAL> GetMaterials();
    std::vector<std::unique_ptr<Texture>> GetTextures();
} // namespace AssimpPerse
