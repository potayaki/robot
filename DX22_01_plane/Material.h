#pragma once
#include	<Windows.h>
#include	<directxmath.h>
#include	<d3d11.h>
#include	<wrl/client.h>
#include	"renderer.h"

class Material {

	struct ConstantBufferMaterial {
		MATERIAL	Material;
	};

	MATERIAL	m_Material{};
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
			sizeof(ConstantBufferMaterial),		// サイズ
			m_pConstantBufferMaterial.GetAddressOf());		// コンスタントバッファ４
		if (!sts) {
			MessageBox(NULL, "CreateBuffer(constant buffer Material) error", "Error", MB_OK);
			return false;
		}

		m_Material.Ambient = mtrl.Ambient;
		m_Material.Diffuse = mtrl.Diffuse;
		m_Material.Speclar = mtrl.Speclar;
		m_Material.Emmision = mtrl.Emmision;
		m_Material.Shiness = mtrl.Shiness;
		m_Material.TextureEnable = mtrl.TextureEnable;

		Update();

		return true;
	}

	void Update() {
		ConstantBufferMaterial		cb{};

		cb.Material = m_Material;

		ID3D11DeviceContext* devcontext;
		devcontext = Renderer::GetDeviceContext();

		devcontext->UpdateSubresource(
			m_pConstantBufferMaterial.Get(),
			0,
			nullptr,
			&cb,
			0, 0);

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
		ConstantBufferMaterial		cb{};

		cb.Material = mtrl;

		ID3D11DeviceContext* devcontext;
		devcontext = Renderer::GetDeviceContext();

		devcontext->UpdateSubresource(
			m_pConstantBufferMaterial.Get(),
			0,
			nullptr,
			&cb,
			0, 0);

		// コンスタントバッファ4をｂ4レジスタへセット（頂点シェーダー用）
		devcontext->VSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());

		// コンスタントバッファ4をｂ4レジスタへセット(ピクセルシェーダー用)
		devcontext->PSSetConstantBuffers(4, 1, m_pConstantBufferMaterial.GetAddressOf());

	}

	void Uninit() {
	}

	void SetDiffuse(DirectX::XMFLOAT4 diffuse) {
		m_Material.Diffuse = diffuse;
	}

	void SetAmbient(DirectX::XMFLOAT4 ambient) {
		m_Material.Ambient = ambient;
	}

	void SetSpecular(DirectX::XMFLOAT4 specular) {
		m_Material.Speclar = specular;
	}

	void SetEmission(DirectX::XMFLOAT4 emission) {
		m_Material.Emmision = emission;
	}

	void SetShiness(float shiness) {
		m_Material.Shiness = shiness;
	}

	bool isTextureEnable() {
		return m_Material.TextureEnable == TRUE;
	}

	void SetColor(float r, float g, float b, float a) {

		// マテリアルのディフューズ（拡散反射光）の色を書き換える

		m_Material.Diffuse.x = r;
		m_Material.Diffuse.y = g;
		m_Material.Diffuse.z = b;
		m_Material.Diffuse.w = a;

		// 変更をGPUに反映させる
		SetMaterial(m_Material);

	}
};