#pragma once
#include "Renderer.h"
#include <wrl/client.h>

class LowHealthGlitch
{
    template<class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D11VertexShader> m_vs;
    Ptr<ID3D11PixelShader> m_ps;
    Ptr<ID3D11Buffer> m_constants;
    Ptr<ID3D11SamplerState> m_sampler;
    Ptr<ID3D11RasterizerState> m_raster;
    Ptr<ID3D11Texture2D> m_capture;
    Ptr<ID3D11ShaderResourceView> m_view;
    bool Init();
public:
    void Draw(float strength, float time);
};
