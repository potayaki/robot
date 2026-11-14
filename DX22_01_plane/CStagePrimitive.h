#pragma once
#include "Object.h"
#include "PrimitiveMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"
#include <cstdint>
#include <memory>

// ステージの見た目を組み立てる部品。移動を止める物理コライダーは持たない。
class CStagePrimitive : public Object {
    struct RenderResources;
    /// 同時に存在する部品で共有するシェーダーと描画ステート。
    static std::weak_ptr<RenderResources> s_sharedResources;
    /// この部品の生存中に共有資源を保持する参照。
    std::shared_ptr<RenderResources> m_resources;
    /// 基本寸法1のローカル座標で生成した頂点。
    VertexBuffer<VERTEX_3D> m_vertices;
    /// 三角形を構成する頂点インデックス。
    IndexBuffer m_indices;
    /// 法線変換・色・UVをシェーダーへ渡す定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constants;
    /// 任意で適用する表面画像。
    Texture m_texture;
    /// 配置コードへ保存する画像ファイルのパス。
    std::string m_texturePath;
    /// 最後の画像読み込み失敗の説明。
    std::string m_textureError;
    /// 生成するプリミティブの種類。
    PrimitiveMesh::Shape m_shape = PrimitiveMesh::Shape::Box;
    /// 球・円錐・円柱の円周方向の分割数。
    int m_slices = PrimitiveMesh::DefaultSlices;
    /// 球の緯度方向の分割数。
    int m_stacks = PrimitiveMesh::DefaultStacks;
    /// 描画するインデックスの個数。
    unsigned m_indexCount = 0;
    /// 裏面も描画するか。
    bool m_doubleSided = false;
    /// UV各方向の画像の繰り返し倍率。
    DirectX::SimpleMath::Vector2 m_uvScale{1, 1};
    /// UV座標に加算する画像のずれ。
    DirectX::SimpleMath::Vector2 m_uvOffset{0, 0};
    /// 表面画像へ乗算する色（各成分0～1）。
    DirectX::SimpleMath::Color m_tint{1, 1, 1, 1};
    /// シーンをまたいで重複しない編集用IDの発行カウンター。
    static inline std::uint64_t s_nextId = 0;
    /// デバッグ画面でこの部品を識別するID。
    const std::uint64_t m_id = ++s_nextId;

    void RebuildMesh();

public:
    ~CStagePrimitive() override {
        Uninit();
    }

    void Init() override;
    void Update() override {};
    void Draw(Camera* camera) override;
    void Uninit() override;

    // 基本寸法は1。SetScale(幅, 高さ, 奥行き)で実際の大きさを指定する。
    bool SetShape(PrimitiveMesh::Shape shape, int slices = PrimitiveMesh::DefaultSlices,
                  int stacks = PrimitiveMesh::DefaultStacks);
    // PNG/JPEG/BMP/TGA等。失敗したらfalseを返し、以前のテクスチャを保持する。空文字で解除。
    bool SetTexture(const std::string& path);
    void ClearTexture();
    void SetTextureTiling(float u, float v);
    void SetTextureOffset(float u, float v);
    void SetTint(const DirectX::SimpleMath::Color& color);

    void SetDoubleSided(bool enabled) {
        m_doubleSided = enabled;
    }

    void SetRotationDegrees(float x, float y, float z);

    PrimitiveMesh::Shape GetShape() const {
        return m_shape;
    }

    const std::string& GetTexturePath() const {
        return m_texturePath;
    }

    const std::string& GetTextureError() const {
        return m_textureError;
    }

    // 調整した部品をStage1Scene::Initへ貼り付けて再現できる。
    std::string PlacementCode() const;
#ifdef _DEBUG
    static void DrawDebugUI();
#endif
};
