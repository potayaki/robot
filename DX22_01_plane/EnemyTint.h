/// @file EnemyTint.h
/// @brief 敵の個体色を描画時だけ適用し、共有モデルや後続描画へ残さない。

#pragma once
#include <SimpleMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

class EnemyTintScope;

class EnemyTint {
public:
    /// 同じデバイスで使用中の着色シェーダーと定数バッファを共有する。
    static std::shared_ptr<EnemyTint> Acquire();

private:
    friend class EnemyTintScope;
    /// GPU資源を作成したデバイス。異なるデバイスの資源を再利用しない。
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    /// テクスチャの明暗を保って個体色へ置き換えるピクセルシェーダー。
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_shader;
    /// 描く個体の色と着色率を送る共有定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_parameters;
};

class EnemyTintScope {
public:
    /// tintのRGBは個体色、Aは着色率。0なら元の描画をそのまま使用する。
    /// textureOnlyは材質を使わないGhost用。表面の透明度には影響しない。
    EnemyTintScope(const std::shared_ptr<EnemyTint>& resources, const DirectX::SimpleMath::Color& tint,
                   bool textureOnly = false);
    ~EnemyTintScope();
    EnemyTintScope(const EnemyTintScope&) = delete;
    EnemyTintScope& operator=(const EnemyTintScope&) = delete;

private:
    /// この描画中だけ使用するデバイスコンテキスト。着色しない場合はnullptr。
    ID3D11DeviceContext* m_context = nullptr;
    /// 呼び出し元が設定していたピクセルシェーダー。
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_previousShader;
    /// 呼び出し元がPSのb7へ設定していた定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_previousParameters;
};
