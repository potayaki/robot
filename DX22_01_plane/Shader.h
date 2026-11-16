/// @file Shader.h
/// @brief 頂点・ピクセルシェーダーの読込と描画時の設定。

#pragma once
#include <wrl/client.h>
#include <memory>
#include <string>
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

/// 描画に使う頂点・ピクセルシェーダーと頂点レイアウトをまとめて扱う。
class Shader {
public:
    /// 現在のデバイスで同じファイルの組を使用中なら、そのプログラムを共有する。
    void Create(std::string vertexShaderPath, std::string pixelShaderPath);
    /// この組を描画に使用する。未作成の場合は対応する GPU の設定を解除する。
    void SetGPU();

private:
    struct Program;
    /// 使用中のプログラムへの所有参照。作成済みの組は各 Shader から読み取り専用にする。
    std::shared_ptr<const Program> m_program;
};
