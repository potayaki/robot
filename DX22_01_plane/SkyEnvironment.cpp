#include "SkyEnvironment.h"
#include "AimCursor.h"
#include "Application.h"
#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace DirectX::SimpleMath;
using SkyWeather::Preset;
using Microsoft::WRL::ComPtr;

namespace
{
    struct SkyDefinition
    {
        unsigned texture;
        Vector3 towardLight;
        Color diffuse;
        Color ambient;
        Vector3 fog;
        Vector4 tint; // RGBは色の倍率、Wは彩度。
        float rotation;
    };

    // 元素材のskies.jsonを基準にした光源方向と色。
    // -Zが前の素材から、ゲームの左手座標へZを反転している。
    // 環境光はゲームの暗部が読める明るさに調整し、朝だけ夕焼け画像を加工する。
    const std::array<SkyDefinition, 5> Definitions = { {
        {1, {-0.9835f, 0.0523f, -0.1734f}, {0.92f, 0.70f, 0.52f, 1},
            {0.48f, 0.42f, 0.40f, 1}, {0.941f, 0.549f, 0.298f},
            {1.03f, 1.15f, 1.32f, 0.55f}, DirectX::XM_PI},
        {0, {0.6124f, 0.7071f, -0.3536f}, {1.0f, 0.957f, 0.863f, 1},
            {0.26f, 0.31f, 0.36f, 1}, {0.753f, 0.847f, 0.918f},
            {1, 1, 1, 1}, 0},
        {1, {-0.9835f, 0.0523f, -0.1734f}, {0.75f, 0.471f, 0.282f, 1},
            {0.45f, 0.30f, 0.25f, 1}, {0.941f, 0.549f, 0.298f},
            {1, 1, 1, 1}, 0},
        {2, {0.2835f, 0.5592f, -0.7790f}, {0.158f, 0.172f, 0.206f, 1},
            {0.23f, 0.26f, 0.34f, 1}, {0.200f, 0.255f, 0.424f},
            {1, 1, 1, 1}, 0},
        {3, {0.3830f, 0.6428f, -0.6634f}, {0.38f, 0.39f, 0.40f, 1},
            {0.43f, 0.46f, 0.50f, 1}, {0.663f, 0.686f, 0.722f},
            {1, 1, 1, 1}, 0}
    } };

    struct SkyConstants
    {
        Vector4 right;
        Vector4 up;
        Vector4 forward;
        Vector4 tintA;
        Vector4 tintB;
        Vector4 parameters;
    };

    struct FogConstants
    {
        Vector4 colorStart;
        Vector4 end;
    };

    Vector4 PackVector(const Vector3& value, float w) {
        return Vector4(value.x, value.y, value.z, w);
    }

    bool CreateBuffer(UINT size, ComPtr<ID3D11Buffer>& buffer) {
        D3D11_BUFFER_DESC description{};
        description.ByteWidth = size;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        return SUCCEEDED(Renderer::GetDevice()->CreateBuffer(&description, nullptr, buffer.GetAddressOf()));
    }

    Vector3 FogColor(const SkyDefinition& definition) {
        // 朝の色調整は空と地平線に同じものを適用する。
        const float gray = definition.fog.Dot(Vector3(0.2126f, 0.7152f, 0.0722f));
        const Vector3 color = Vector3::Lerp(Vector3(gray), definition.fog, definition.tint.w);
        return color * Vector3(definition.tint.x, definition.tint.y, definition.tint.z);
    }
}

bool SkyEnvironment::Init() {
    Uninit();
    const char* files[] = { "day_fair.png", "sunset_orange.png", "night_full_moon.png", "overcast_grey.png" };
    for (unsigned i = 0; i < m_textures.size(); ++i) {
        if (!m_textures[i].Load(std::string("assets/sky/low-poly-skies-free/") + files[i])) return false;
    }

    // 画面全体を覆う三角形。空用のモデルは不要で、カメラの向きから360度画像を引く。
    std::vector<VERTEX_3D> vertices(3);
    vertices[0].position = Vector3(-1, -1, 0);
    vertices[1].position = Vector3(-1, 3, 0);
    vertices[2].position = Vector3(3, -1, 0);
    m_vertices.Create(vertices);
    D3D11_INPUT_ELEMENT_DESC layout = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    if (FAILED(Renderer::CreateVertexShader(m_vertexShader.GetAddressOf(), m_inputLayout.GetAddressOf(),
        &layout, 1, "shader/skyVS.hlsl"))) return false;
    if (FAILED(Renderer::CreatePixelShader(m_pixelShader.GetAddressOf(), "shader/skyPS.hlsl"))) return false;
    if (!CreateBuffer(sizeof(SkyConstants), m_skyBuffer) || !CreateBuffer(sizeof(FogConstants), m_fogBuffer)) return false;

    // 横方向は一周してつながり、縦方向は天頂・真下で止める。
    D3D11_SAMPLER_DESC sampler{};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(Renderer::GetDevice()->CreateSamplerState(&sampler, m_sampler.GetAddressOf()))) return false;

    // 空は一番最初に描き、深度には書かない。後から地面・敵・HUDを普通に描ける。
    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
    if (FAILED(Renderer::GetDevice()->CreateDepthStencilState(&depth, m_depth.GetAddressOf()))) return false;
    D3D11_RASTERIZER_DESC rasterizer{};
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = TRUE;
    if (FAILED(Renderer::GetDevice()->CreateRasterizerState(&rasterizer, m_rasterizer.GetAddressOf()))) return false;

    m_cycle.RollWeather([this] { return Random01(); });
    SetTarget(m_cycle.Current(), false);
    m_ready = true;
    return true;
}

float SkyEnvironment::Random01() {
    // 敵の出現などに使う乱数とは分け、天候の抽選でゲーム側の結果を変えない。
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(m_random);
}

void SkyEnvironment::SetTarget(Preset preset, bool blend) {
    if (!blend) {
        // Debugで選んだときはすぐ確認できるよう、切替待ちなしで固定表示する。
        m_previous = m_current = preset;
        m_transitionSeconds = 8.0f;
    }
    else if (preset != m_current) {
        m_previous = m_current;
        m_current = preset;
        m_transitionSeconds = 0.0f;
    }
}

void SkyEnvironment::Update(float deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) return;
    if (m_automatic) {
        m_cycle.Update(deltaSeconds, [this] { return Random01(); });
        SetTarget(m_cycle.Current(), true);
    }
    m_transitionSeconds = (std::min)(m_transitionSeconds + deltaSeconds, 8.0f);
}

void SkyEnvironment::Draw(Camera* camera) {
    if (!m_ready || !camera) return;
    auto context = Renderer::GetDeviceContext();
    camera->SetCamera();

    const auto& a = Definitions[static_cast<size_t>(m_previous)];
    const auto& b = Definitions[static_cast<size_t>(m_current)];
    const float blend = SkyWeather::TransitionWeight(m_transitionSeconds);
    const float rotation = DirectX::XMConvertToRadians(m_rotationDegrees);

    // HUDが前フレームに設定した行列を使わず、3Dカメラを更新してから方向だけを取り出す。
    const Matrix inverseView = camera->GetViewMatrix().Invert();
    const float tangent = std::tan(AimCursor::FieldOfView * 0.5f);
    const float aspect = static_cast<float>(Application::GetWidth()) / (std::max)(1u, Application::GetHeight());
    SkyConstants sky{};
    sky.right = PackVector(Vector3::TransformNormal(Vector3(tangent * aspect, 0, 0), inverseView), 0);
    sky.up = PackVector(Vector3::TransformNormal(Vector3(0, tangent, 0), inverseView), 0);
    sky.forward = PackVector(Vector3::TransformNormal(Vector3(0, 0, 1), inverseView), 0);
    sky.tintA = a.tint;
    sky.tintB = b.tint;
    sky.parameters = Vector4(blend, m_skyBrightness, rotation + a.rotation, rotation + b.rotation);
    context->UpdateSubresource(m_skyBuffer.Get(), 0, nullptr, &sky, 0, 0);

    // 太陽・月の向きも空の回転と一致させ、地形とロボットに同じ光を当てる。
    Vector3 directionA = Vector3::TransformNormal(a.towardLight, Matrix::CreateRotationY(rotation + a.rotation));
    Vector3 directionB = Vector3::TransformNormal(b.towardLight, Matrix::CreateRotationY(rotation + b.rotation));
    Vector3 direction = -Vector3::Lerp(directionA, directionB, blend);
    direction.Normalize();
    LIGHT light{};
    light.Enable = TRUE;
    light.Direction = PackVector(direction, 0);
    light.Diffuse = Color::Lerp(a.diffuse, b.diffuse, blend) * m_sceneBrightness;
    light.Ambient = Color::Lerp(a.ambient, b.ambient, blend) * m_sceneBrightness;
    Renderer::SetLight(light);

    // 近くの戦闘は霞ませず、遠くの地面だけを地平線の色へ近づける。
    const Vector3 fog = Vector3::Lerp(FogColor(a), FogColor(b), blend) * m_skyBrightness;
    FogConstants fogConstants{ PackVector(fog, 600.0f), Vector4(3000.0f, 0, 0, 0) };
    context->UpdateSubresource(m_fogBuffer.Get(), 0, nullptr, &fogConstants, 0, 0);
    context->PSSetConstantBuffers(8, 1, m_fogBuffer.GetAddressOf());

    // 空専用の状態を保存・復元し、透明エフェクトやミニマップの設定へ持ち越さない。
    ComPtr<ID3D11BlendState> previousBlend;
    ComPtr<ID3D11DepthStencilState> previousDepth;
    ComPtr<ID3D11RasterizerState> previousRasterizer;
    ComPtr<ID3D11SamplerState> previousSampler;
    ComPtr<ID3D11Buffer> previousVertexConstants, previousPixelConstants;
    ComPtr<ID3D11ShaderResourceView> previousTextures[2];
    float blendFactor[4]{};
    UINT sampleMask = 0, stencil = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->OMGetBlendState(previousBlend.GetAddressOf(), blendFactor, &sampleMask);
    context->OMGetDepthStencilState(previousDepth.GetAddressOf(), &stencil);
    context->RSGetState(previousRasterizer.GetAddressOf());
    context->PSGetSamplers(0, 1, previousSampler.GetAddressOf());
    context->VSGetConstantBuffers(7, 1, previousVertexConstants.GetAddressOf());
    context->PSGetConstantBuffers(7, 1, previousPixelConstants.GetAddressOf());
    for (UINT i = 0; i < 2; ++i) context->PSGetShaderResources(i, 1, previousTextures[i].GetAddressOf());
    context->IAGetPrimitiveTopology(&topology);

    ID3D11ShaderResourceView* textures[] = {
        m_textures[a.texture].GetShaderResourceView(), m_textures[b.texture].GetShaderResourceView() };
    Renderer::SetBlendState(BS_NONE);
    context->OMSetDepthStencilState(m_depth.Get(), 0);
    context->RSSetState(m_rasterizer.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    m_vertices.SetGPU();
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(7, 1, m_skyBuffer.GetAddressOf());
    context->PSSetConstantBuffers(7, 1, m_skyBuffer.GetAddressOf());
    context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
    context->PSSetShaderResources(0, 2, textures);
    context->Draw(3, 0);

    context->OMSetBlendState(previousBlend.Get(), blendFactor, sampleMask);
    context->OMSetDepthStencilState(previousDepth.Get(), stencil);
    context->RSSetState(previousRasterizer.Get());
    context->PSSetSamplers(0, 1, previousSampler.GetAddressOf());
    context->VSSetConstantBuffers(7, 1, previousVertexConstants.GetAddressOf());
    context->PSSetConstantBuffers(7, 1, previousPixelConstants.GetAddressOf());
    for (UINT i = 0; i < 2; ++i) context->PSSetShaderResources(i, 1, previousTextures[i].GetAddressOf());
    context->IASetPrimitiveTopology(topology);
    // 頂点・シェーダー・レイアウトは、この後に描く各モデルが自身のものを設定する。
}

#ifdef _DEBUG
void SkyEnvironment::DrawDebugUI() {
    auto text = [](const char8_t* value) { return reinterpret_cast<const char*>(value); };
    if (!ImGui::CollapsingHeader(text(u8"空・天候"), ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (!m_ready) {
        ImGui::TextUnformatted(text(u8"空の読み込みに失敗しました。assets/sky と shader を確認してください。"));
        return;
    }

    if (ImGui::Checkbox(text(u8"時間で自動切替"), &m_automatic)) {
        // 停止中のDebugでも、チェック操作の結果をその場で確認できるようにする。
        SetTarget(m_automatic ? m_cycle.Current() : m_current, false);
    }
    const char8_t* names[] = { u8"朝", u8"昼", u8"夕方", u8"夜", u8"曇り" };
    for (int i = 0; i < static_cast<int>(Preset::Count); ++i) {
        if (i != 0) ImGui::SameLine();
        const auto preset = static_cast<Preset>(i);
        if (ImGui::RadioButton(text(names[i]), m_current == preset)) {
            m_automatic = false;
            SetTarget(preset, false);
        }
    }
    ImGui::TextWrapped("%s", text(u8"選ぶと固定表示。自動切替ONで時間が進みます。"));

    ImGui::PushItemWidth((std::min)(170.0f, ImGui::GetContentRegionAvail().x * 0.45f));
    float minutes = m_cycle.durationSeconds / 60.0f;
    if (ImGui::SliderFloat(text(u8"1周の時間（分）"), &minutes, 1.0f, 20.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
        m_cycle.durationSeconds = minutes * 60.0f;
    float probability = m_cycle.cloudyProbability * 100.0f;
    if (ImGui::SliderFloat(text(u8"朝・昼に曇る確率"), &probability, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
        m_cycle.cloudyProbability = probability / 100.0f;
    ImGui::Text(text(u8"進行中の時間帯: %s（残り %.0f 秒）"), text(names[static_cast<int>(m_cycle.TimeOfDay())]),
        (1.0f - m_cycle.Progress()) * m_cycle.durationSeconds / 4.0f);
    if (ImGui::Button(text(u8"次の時間帯へ"))) {
        m_cycle.Next([this] { return Random01(); });
        m_automatic = true;
        SetTarget(m_cycle.Current(), false);
    }
    ImGui::SliderFloat(text(u8"空の向き"), &m_rotationDegrees, -180.0f, 180.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"空の明るさ"), &m_skyBrightness, 0.5f, 1.5f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"地形・モデルの明るさ"), &m_sceneBrightness, 0.5f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();
    ImGui::Separator();
}
#endif

void SkyEnvironment::Uninit() {
    m_ready = false;
    m_textures = {};
    m_vertices = {};
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_inputLayout.Reset();
    m_skyBuffer.Reset();
    m_fogBuffer.Reset();
    m_sampler.Reset();
    m_depth.Reset();
    m_rasterizer.Reset();
}
