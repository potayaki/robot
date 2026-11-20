/// @file EnemyTint.cpp
/// @brief 共有された敵描画資源へ個体ごとの色を安全に適用する。

#include "EnemyTint.h"
#include "Renderer.h"

std::shared_ptr<EnemyTint> EnemyTint::Acquire() {
    static std::weak_ptr<EnemyTint> cached;
    auto resources = cached.lock();
    auto device = Renderer::GetDevice();
    if (resources && resources->m_device.Get() == device) {
        return resources;
    }
    resources = std::make_shared<EnemyTint>();
    resources->m_device = device;
    if (FAILED(Renderer::CreatePixelShader(resources->m_shader.GetAddressOf(), "shader/enemyTintPS.hlsl"))) {
        return {};
    }
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(float) * 8;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&desc, nullptr, resources->m_parameters.GetAddressOf()))) {
        return {};
    }
    cached = resources;
    return resources;
}

EnemyTintScope::EnemyTintScope(const std::shared_ptr<EnemyTint>& resources,
                               const DirectX::SimpleMath::Color& tint, bool textureOnly) {
    if (!resources || tint.w <= 0) {
        return;
    }
    m_context = Renderer::GetDeviceContext();
    m_context->PSGetShader(m_previousShader.GetAddressOf(), nullptr, nullptr);
    m_context->PSGetConstantBuffers(7, 1, m_previousParameters.GetAddressOf());
    const float parameters[]{tint.x, tint.y, tint.z, tint.w, textureOnly ? 1.0f : 0.0f, 0, 0, 0};
    m_context->UpdateSubresource(resources->m_parameters.Get(), 0, nullptr, parameters, 0, 0);
    m_context->PSSetConstantBuffers(7, 1, resources->m_parameters.GetAddressOf());
    m_context->PSSetShader(resources->m_shader.Get(), nullptr, 0);
}

EnemyTintScope::~EnemyTintScope() {
    if (m_context) {
        m_context->PSSetShader(m_previousShader.Get(), nullptr, 0);
        m_context->PSSetConstantBuffers(7, 1, m_previousParameters.GetAddressOf());
    }
}
