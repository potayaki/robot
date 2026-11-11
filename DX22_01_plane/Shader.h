/// @file Shader.h
/// @brief 頂点・ピクセルシェーダーの読込と描画時の設定。

#pragma once
#include <wrl/client.h>
#include <string>
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

//-----------------------------------------------------------------------------
//Shaderクラス
//-----------------------------------------------------------------------------
class Shader {
public:
    void Create(std::string vs, std::string ps);
    void SetGPU();

private:
    /// 頂点シェーダー
    ComPtr<ID3D11VertexShader> m_pVertexShader;
    /// ピクセルシェーダー
    ComPtr<ID3D11PixelShader> m_pPixelShader;
    /// 頂点レイアウト
    ComPtr<ID3D11InputLayout> m_pVertexLayout;
};
