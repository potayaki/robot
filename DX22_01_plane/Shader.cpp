/// @file Shader.cpp
/// @brief 頂点・ピクセルシェーダーの読込と描画時の設定。

#include "Shader.h"
#include "renderer.h"
#include "Application.h"
#include <algorithm>
#include <mutex>
#include <utility>
#include <vector>

struct Shader::Program {
    /// このプログラムを作成したデバイス。別のデバイスとは GPU 資源を共有できない。
    ComPtr<ID3D11Device> m_device;
    /// 頂点シェーダーの読み込み元。ピクセル側のパスと組にして照合する。
    std::string m_vertexShaderPath;
    /// ピクセルシェーダーの読み込み元。
    std::string m_pixelShaderPath;
    /// 描画時に頂点ステージに設定するシェーダー。
    ComPtr<ID3D11VertexShader> m_vertexShader;
    /// 描画時にピクセルステージに設定するシェーダー。
    ComPtr<ID3D11PixelShader> m_pixelShader;
    /// メッシュの各頂点属性と頂点シェーダーの入力との対応。
    ComPtr<ID3D11InputLayout> m_inputLayout;

    /// この組をキャッシュに登録する前に、描画に必要な GPU 資源をすべて作成する。
    bool CreateGpuResources();
};

void Shader::Create(std::string vertexShaderPath, std::string pixelShaderPath) {
    // 同じ組が同時に要求されても、検索から登録までに作成するプログラムは一つ。
    static std::mutex cacheMutex;
    // 所有権は利用中の Shader にある。次の検索時に期限切れの弱参照を取り除く。
    static std::vector<std::weak_ptr<const Program>> cachedPrograms;
    const std::scoped_lock lock(cacheMutex);
    std::erase_if(cachedPrograms, [](const auto& entry) { return entry.expired(); });
    ID3D11Device* device = Renderer::GetDevice();
    for (const auto& entry : cachedPrograms) {
        auto program = entry.lock();
        if (program && program->m_device.Get() == device && program->m_vertexShaderPath == vertexShaderPath &&
            program->m_pixelShaderPath == pixelShaderPath) {
            m_program = std::move(program);
            return;
        }
    }

    auto program = std::make_shared<Program>();
    program->m_device = device;
    program->m_vertexShaderPath = std::move(vertexShaderPath);
    program->m_pixelShaderPath = std::move(pixelShaderPath);
    if (program->CreateGpuResources()) {
        cachedPrograms.emplace_back(program);
        m_program = std::move(program);
    }
}

bool Shader::Program::CreateGpuResources() {
    // 頂点バッファに格納される順に、位置・法線・色・UV の形式を指定する。
    D3D11_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
         D3D11_INPUT_PER_VERTEX_DATA, 0}};

    if (FAILED(Renderer::CreateVertexShader(&m_vertexShader, &m_inputLayout, inputElements,
                                            ARRAYSIZE(inputElements), m_vertexShaderPath.c_str()))) {
        MessageBox(Application::GetWindow(), "CreateVertexShader error", "error", MB_OK);
        return false;
    }

    if (FAILED(Renderer::CreatePixelShader(&m_pixelShader, m_pixelShaderPath.c_str()))) {
        MessageBox(nullptr, "CreatePixelShader error", "error", MB_OK);
        return false;
    }
    return true;
}

void Shader::SetGPU() {
    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();

    deviceContext->VSSetShader(m_program ? m_program->m_vertexShader.Get() : nullptr, nullptr, 0);
    deviceContext->PSSetShader(m_program ? m_program->m_pixelShader.Get() : nullptr, nullptr, 0);
    deviceContext->IASetInputLayout(m_program ? m_program->m_inputLayout.Get() : nullptr);
}
