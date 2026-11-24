/// @file StageSunShadows.h
/// @brief 太陽から見たプレイヤー周辺の深度を記録し、森の地面に投影する。

#pragma once

#include "Renderer.h"
#include <functional>
#include <wrl/client.h>

class StageSunShadows {
public:
    /// 深度マップ、比較サンプラー、地面へ渡す定数を作成する。
    bool Init();
    /// ワールド座標の中心と光線の進行方向から影を描く。終了後は呼び出し側でカメラを戻す。
    void Capture(const DirectX::SimpleMath::Vector3& center,
                 const DirectX::SimpleMath::Vector3& lightDirection, const std::function<void()>& drawCaster);
    /// 森の地面を描くピクセルシェーダーに影を設定する。
    void BindGround() const;
    /// 地面の描画後に深度マップの読み取りを終了する。
    void UnbindGround() const;

private:
    /// 太陽から見た最前面の深度を書き込むビュー。
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depth;
    /// 同じ深度を地面のシェーダーから読み取るビュー。
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_map;
    /// 太陽のビュー射影行列、マップの解像度、深度比較条件。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constants;
    /// 地面の深度とマップを比較し、境界の外側を日向にするサンプラー。
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_comparison;
    /// 片面のモデルも影に含める深度描画用の塗りつぶし設定。
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer;
};
