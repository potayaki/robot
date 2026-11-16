#pragma once
#include "Renderer.h"
#include <functional>
#include <wrl/client.h>

// 固定照明から台座上のフィギュアを描き、足元と空中の敵の投影影を作る。
class TitleFigureShadows {
public:
    bool Init();
    void Capture(const std::function<void()>& drawCasters);
    void BindGround() const;
    void UnbindGround() const;

private:
    DirectX::SimpleMath::Matrix m_view, m_projection;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depth;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_map;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightMatrix;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_comparison;
    bool m_ready = false;
};
