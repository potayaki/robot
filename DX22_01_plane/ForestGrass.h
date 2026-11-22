/// @brief 森の草の葉を、カメラに近い区画ごとにインスタンス描画する。
#pragma once
#include <SimpleMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

class Camera;

class ForestGrass {
    struct Tile {
        /// 風で揺れる葉先まで含む境界球のワールド中心。
        DirectX::SimpleMath::Vector3 center;
        /// 境界球の半径（ワールド単位）。
        float radius;
        /// この区画の先頭インスタンス。
        UINT first;
        /// この区画にある葉の枚数。
        UINT count;
    };

    /// 空の区画を除いた描画範囲。一つの頂点バッファを全区画で共有する。
    std::vector<Tile> m_tiles;
    /// 各葉の根元・寸法・向きを一度だけ転送するバッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instances;
    /// インスタンスと頂点番号から葉の曲面を作るシェーダー。
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    /// 葉の陰影を描くシェーダー。
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    /// 根元と形状の2属性を持つインスタンスの入力形式。
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout;
    /// 薄い葉の裏面も描画する。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;

public:
    bool Init();
    void Draw(Camera* camera) const;
};
