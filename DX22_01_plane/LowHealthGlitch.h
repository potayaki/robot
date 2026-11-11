/// @file LowHealthGlitch.h
/// @brief 低HP時の画面ノイズ演出と、描画結果の取り込み。

#pragma once
#include "Renderer.h"
#include <wrl/client.h>

class LowHealthGlitch {
    template <class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    /// 画面演出用の頂点シェーダー。
    Ptr<ID3D11VertexShader> m_vs;
    /// 画面演出用のピクセルシェーダー。
    Ptr<ID3D11PixelShader> m_ps;
    /// 画面演出の強さや時間をGPUへ渡す定数バッファ。
    Ptr<ID3D11Buffer> m_constants;
    /// テクスチャを拡大縮小するときの参照方法。
    Ptr<ID3D11SamplerState> m_sampler;
    /// 画面演出のポリゴンを描画する設定。
    Ptr<ID3D11RasterizerState> m_raster;
    /// 画面演出の入力に使用する描画結果のコピー。
    Ptr<ID3D11Texture2D> m_capture;
    /// コピーした画面をシェーダーから参照するためのビュー。
    Ptr<ID3D11ShaderResourceView> m_view;
    /// 使用前の状態と必要な資源を初期化する。
    bool Init();

public:
    /// 現在の状態を画面へ描画する。
    void Draw(float strength, float time);
};
