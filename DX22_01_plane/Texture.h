/// @file Texture.h
/// @brief 画像の読込、ミップマップ生成、シェーダーへの設定。

#pragma once

#include <d3d11.h>
#include <string>
#include <wrl/client.h> // ComPtrの定義を含むヘッダファイル
#include <filesystem>

using Microsoft::WRL::ComPtr;

//-----------------------------------------------------------------------------
//Textureクラス
//-----------------------------------------------------------------------------
class Texture {
    /// ファイル名
    std::string m_texname{};
    /// シェーダーリソースビュー
    ComPtr<ID3D11ShaderResourceView> m_srv{};

    /// 幅
    int m_width;
    /// 高さ
    int m_height;
    /// BPP
    int m_bpp;

public:
    bool Load(const std::string& filename);
    bool LoadFromFemory(const unsigned char* data, int len);

    void SetGPU();

    // 空の切替時に2枚の画像を同時に参照するための読み取り専用ビュー。
    ID3D11ShaderResourceView* GetShaderResourceView() const {
        return m_srv.Get();
    }
};