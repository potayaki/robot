/// @file Material.h
/// @brief 材質パラメータを保持してGPUへ転送する処理。

#pragma once
#include <Windows.h>
#include <directxmath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include "renderer.h"

class Material {
    struct ConstantBufferMaterial {
        /// GPUへ転送する材質データ。
        MATERIAL Material;
    };

    /// 描画時に使用する色・反射・テクスチャ有効状態。
    MATERIAL m_material{};
    /// 材質の各パラメータをGPUへ渡す定数バッファ。
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pConstantBufferMaterial;

public:
    Material() {}

    Material(MATERIAL mtrl) {
        Create(mtrl);
    }

    ~Material() {
        Uninit();
    }

    bool Create(MATERIAL mtrl) {
        ID3D11Device* dev;
        dev = Renderer::GetDevice();

        // コンスタントバッファ作成
        bool sts = Renderer::CreateConstantBuffer(
            sizeof(ConstantBufferMaterial), // サイズ
            m_pConstantBufferMaterial.GetAddressOf()); // コンスタントバッファ４
        if (!sts) {
            MessageBox(NULL, "CreateBuffer(constant buffer Material) error", "Error", MB_OK);
            return false;
        }

        m_material.Ambient = mtrl.Ambient;
        m_material.Diffuse = mtrl.Diffuse;
        m_material.Speclar = mtrl.Speclar;
        m_material.Emmision = mtrl.Emmision;
        m_material.Shiness = mtrl.Shiness;
        m_material.TextureEnable = mtrl.TextureEnable;

        Update();

        return true;
    }

    /// 経過時間と入力に合わせて状態を更新する。
    void Update() {
        ConstantBufferMaterial cb{};

        cb.Material = m_material;

        ID3D11DeviceContext* devcontext;
        devcontext = Renderer::GetDeviceContext();

        devcontext->UpdateSubresource(m_pConstantBufferMaterial.Get(), 0, nullptr, &cb, 0, 0);

        // コンスタントバッファ4をｂ4レジスタへセット（頂点シェーダー用）
        devcontext->VSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());

        // コンスタントバッファ4をｂ4レジスタへセット(ピクセルシェーダー用)
        devcontext->PSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());
    }

    void SetGPU() {
        ID3D11DeviceContext* devcontext;
        devcontext = Renderer::GetDeviceContext();

        // コンスタントバッファ4をｂ4レジスタへセット（頂点シェーダー用）
        devcontext->VSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());

        // コンスタントバッファ4をｂ4レジスタへセット(ピクセルシェーダー用)
        devcontext->PSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());
    }

    void SetMaterial(const MATERIAL& mtrl) {
        ConstantBufferMaterial cb{};

        cb.Material = mtrl;

        ID3D11DeviceContext* devcontext;
        devcontext = Renderer::GetDeviceContext();

        devcontext->UpdateSubresource(m_pConstantBufferMaterial.Get(), 0, nullptr, &cb, 0, 0);

        // コンスタントバッファ4をｂ4レジスタへセット（頂点シェーダー用）
        devcontext->VSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());

        // コンスタントバッファ4をｂ4レジスタへセット(ピクセルシェーダー用)
        devcontext->PSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());
    }

    /// 使用した資源を解放して終了する。
    void Uninit() {}

    void SetDiffuse(DirectX::XMFLOAT4 diffuse) {
        m_material.Diffuse = diffuse;
    }

    void SetAmbient(DirectX::XMFLOAT4 ambient) {
        m_material.Ambient = ambient;
    }

    void SetSpecular(DirectX::XMFLOAT4 specular) {
        m_material.Speclar = specular;
    }

    void SetEmission(DirectX::XMFLOAT4 emission) {
        m_material.Emmision = emission;
    }

    void SetShiness(float shiness) {
        m_material.Shiness = shiness;
    }

    bool isTextureEnable() {
        return m_material.TextureEnable == TRUE;
    }

    void SetColor(float r, float g, float b, float a) {
        // マテリアルのディフューズ（拡散反射光）の色を書き換える

        m_material.Diffuse.x = r;
        m_material.Diffuse.y = g;
        m_material.Diffuse.z = b;
        m_material.Diffuse.w = a;

        // 変更をGPUに反映させる
        SetMaterial(m_material);
    }
};