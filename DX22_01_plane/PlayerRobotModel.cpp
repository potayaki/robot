/// @file PlayerRobotModel.cpp
/// @brief プレイヤーのロボット表示、足のIK、死亡時の分解表示。

#include "PlayerRobotModel.h"

#include <cstddef>
#include <iostream>
#include <iterator>

#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace DirectX::SimpleMath;

namespace {
    constexpr float RobotScale = 0.025f; // 素材全体をゲーム内サイズへ変換する倍率。
    constexpr float LowerBodyScale = 1.40f; // 腰と6本脚だけを拡大。上半身の大きさは保つ。
    constexpr float PlayerCollisionRadius = 10.0f; // 判定の中心から底面までモデルを下げる量。
    constexpr unsigned BoneBufferSlot = 6; // playerRobotVS.hlslのregister(b6)に対応する番号。

    Matrix ToDirectX(const aiMatrix4x4& matrix) {
        // Assimpは列ベクトル、SimpleMathは行ベクトルなので転置して渡す。
        return Matrix(matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2,
                      matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4);
    }

    aiMatrix4x4 ToAssimp(const Matrix& matrix) {
        return aiMatrix4x4(matrix._11, matrix._21, matrix._31, matrix._41, matrix._12, matrix._22, matrix._32,
                           matrix._42, matrix._13, matrix._23, matrix._33, matrix._43, matrix._14, matrix._24,
                           matrix._34, matrix._44);
    }
} // namespace

PlayerRobotModel::SkinnedVertex PlayerRobotModel::MakeVertex(const RobotAnimationRig::Vertex& source) {
    SkinnedVertex vertex{};
    vertex.vertex.position = Vector3(source.position.x, source.position.y, source.position.z);
    vertex.vertex.normal = Vector3(source.normal.x, source.normal.y, source.normal.z);
    vertex.vertex.uv = Vector2(source.uv.x, source.uv.y);
    vertex.vertex.color = Color(1, 1, 1, 1); // 白を掛け、テクスチャ本来の色を使う。
    vertex.bone = source.node;
    return vertex;
}

void PlayerRobotModel::Init() {
    Uninit();

    std::string error; // 素材の読込に失敗した場合の説明。
    if (!m_rig.Load("assets/model/Player", error)) {
        std::cerr << "Player robot: " << error << '\n';
        return;
    }

    if (m_rig.GetMatrices().size() > MaxBones) {
        return;
    }

    // 先に下半身を拡大し、IKが使う脚の長さと基準接地点にも同じ倍率を反映する。
    m_rig.SetLowerBodyScale(LowerBodyScale);
    m_ikReady = m_spiderIK.Init(m_rig);
    if (!m_ikReady) {
        std::cerr << "Player six-leg IK could not find the expected joints.\n";
    }

    // FBX内の画像参照は配布フォルダと一致しないため、付属のBase Colorを直接指定する。
    if (!m_albedo.Load("assets/model/Player/Textures/T_Mech_Base_Color.png")) {
        return;
    }

    std::vector<SkinnedVertex> vertices; // 各パーツの頂点を1本の描画バッファにまとめる。
    for (const auto& part : m_rig.GetParts()) {
        for (const auto& source : part.vertices) {
            vertices.push_back(MakeVertex(source));
        }
    }
    m_vertices.Create(vertices);
    m_vertexCount = static_cast<unsigned>(vertices.size());

    // 頂点内のバイト位置は構造体から求める。項目のサイズを変更しても数字の修正が不要になる。
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         offsetof(SkinnedVertex, vertex) + offsetof(VERTEX_3D, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         offsetof(SkinnedVertex, vertex) + offsetof(VERTEX_3D, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         offsetof(SkinnedVertex, vertex) + offsetof(VERTEX_3D, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         offsetof(SkinnedVertex, vertex) + offsetof(VERTEX_3D, uv), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, offsetof(SkinnedVertex, bone),
         D3D11_INPUT_PER_VERTEX_DATA, 0}};

    if (FAILED(Renderer::CreateVertexShader(m_vertexShader.GetAddressOf(), m_layout.GetAddressOf(), layout,
                                            static_cast<unsigned>(std::size(layout)),
                                            "shader/playerRobotVS.hlsl"))) {
        return;
    }

    if (FAILED(Renderer::CreatePixelShader(m_pixelShader.GetAddressOf(), "shader/litTexturePS.hlsl"))) {
        return;
    }

    const auto boneBufferBytes = static_cast<unsigned>(sizeof(Matrix) * MaxBones);
    if (!Renderer::CreateConstantBuffer(boneBufferBytes, m_boneBuffer.GetAddressOf())) {
        return;
    }

    m_ready = true;
}

void PlayerRobotModel::Advance(RobotMotion motion, float deltaSeconds, float playbackRate, float upperBodyYaw,
                               float upperBodyPitch, float groundY, bool grounded, bool procedural) {
    if (!m_ready) {
        return;
    }

    const bool useIK = m_ikReady && procedural;
    m_rig.Update(useIK ? RobotMotion::Idle : motion, deltaSeconds, playbackRate, upperBodyYaw,
                 upperBodyPitch);

    if (useIK) {
        m_spiderIK.Update(m_rig, deltaSeconds, ToAssimp(GetWorldMatrix()), groundY, grounded);
    } else {
        // 素材のアニメーションをプレビューした後は、古い接地点を使わずIKを再開する。
        m_spiderIK.Reset();
    }
}

#ifdef _DEBUG
void PlayerRobotModel::DrawIKDebugUI() {
    if (!m_ready || !m_ikReady || m_debris.IsActive()) {
        return;
    }

    using Limits = RobotSpiderIK::TuningLimits; // 内部の入力チェックと同じ上下限を使用する。
    const auto text = [](const char8_t* value) {
        return reinterpret_cast<const char*>(value); // ImGuiへUTF-8文字列を渡す。
    };
    auto tuning = m_spiderIK.m_settings; // そのフレームの編集内容。変更後にまとめてIKへ渡す。
    bool changed = false; // スライダーや初期化ボタンで値が変更されたか。

    ImGui::PushID("SpiderIK");
    ImGui::Separator();
    ImGui::TextUnformatted(text(u8"6本脚のIK調整"));

    changed |= ImGui::SliderFloat(text(u8"歩幅"), &tuning.strideLength, Limits::MinStrideLength,
                                  Limits::MaxStrideLength, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text(u8"同じ足の着地から次の着地までの希望距離。大きくすると大股になります。"
                                     u8"脚の長さに合う範囲で調整してください。"));
    }

    changed |= ImGui::SliderFloat(text(u8"足の開き"), &tuning.stanceWidth, Limits::MinStanceWidth,
                                  Limits::MaxStanceWidth, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "%s", text(u8"大きいほど足先を外へ広げます。1.00が素材の元の接地幅です。停止中も反映されます。"));
    }

    changed |= ImGui::SliderFloat(text(u8"腰の高さ"), &tuning.bodyHeight, Limits::MinBodyHeight,
                                  Limits::MaxBodyHeight, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "%s", text(u8"足先を接地させたまま腰を上下します。足の開きと組み合わせて脚の傾きを調整します。"));
    }

    changed |= ImGui::SliderFloat(text(u8"足を上げる高さ"), &tuning.stepHeight, Limits::MinStepHeight,
                                  Limits::MaxStepHeight, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text(u8"一歩の途中で足を持ち上げる高さです。接地中の足の位置は変えません。"));
    }

    if (ImGui::Button(text(u8"IK設定を初期値に戻す"))) {
        tuning = RobotSpiderIK::Settings{};
        changed = true;
    }

    if (changed) {
        // 時間を進めず骨行列だけ作り直す。一時停止中でも足の開きや腰の高さを確認できる。
        m_spiderIK.ApplySettings(m_rig, tuning);
    }

    ImGui::TextWrapped("%s",
                       text(u8"変更は実行中のみ有効です。足の開き・腰の高さは一時停止中も確認できます。"));
    if (m_spiderIK.GetContactError() > Limits::ContactWarningDistance) {
        ImGui::TextWrapped(
            "%s",
            text(u8"脚の届く範囲を超えています。歩幅・足の開き・腰の高さの組み合わせを調整してください。"));
    }
    ImGui::PopID();
}
#endif

void PlayerRobotModel::BeginBreakApart(float groundY) {
    if (!m_ready || m_debris.IsActive()) {
        return;
    }

    // 照準と歩行を含む「今の姿勢」を、ワールド座標の破片へ焼き付ける。
    m_debris.Start(m_rig, ToAssimp(GetWorldMatrix()), groundY);
    if (!m_debris.IsActive()) {
        return;
    }

    if (m_debris.GetMatrices().size() > MaxBones) {
        m_debris.Reset();
        return;
    }

    std::vector<SkinnedVertex> vertices;
    vertices.reserve(m_debris.GetVertices().size());
    for (const auto& vertex : m_debris.GetVertices()) {
        vertices.push_back(MakeVertex(vertex));
    }
    m_debrisVertices.Create(vertices);
}

void PlayerRobotModel::AdvanceBreakApart(float deltaSeconds) {
    m_debris.Update(deltaSeconds);
}

void PlayerRobotModel::RestoreBody() {
    if (!m_debris.IsActive()) {
        return;
    }

    m_debris.Reset();
    m_debrisVertices = VertexBuffer<SkinnedVertex>{};
    m_spiderIK.Reset();
}

Matrix PlayerRobotModel::GetWorldMatrix() const {
    // 素材の足元はY=0、正面は-Z。判定の底面まで下げ、ゲームの+Zへ正面を合わせる。
    // m_Rotationは脚側の向き。照準へ向ける上半身の回転は骨行列にだけ加える。
    const auto scale = Matrix::CreateScale(RobotScale);
    const auto rotation = Matrix::CreateRotationY(m_rotation.y + DirectX::XM_PI);
    const auto translation = Matrix::CreateTranslation(m_position - Vector3(0, PlayerCollisionRadius, 0));
    return scale * rotation * translation;
}

void PlayerRobotModel::Draw(Camera* cam) {
    if (!m_ready || !cam) {
        return;
    }

    cam->SetCamera();

    // 破片は既にワールド座標。本体と二重に描かず、同じ材質・シェーダーで切り替える。
    const bool broken = m_debris.IsActive();
    Matrix world = broken ? Matrix::Identity : GetWorldMatrix();
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1); // テクスチャ全体を使用する。

    MATERIAL material{};
    material.Diffuse = Color(1, 1, 1, 1);
    material.TextureEnable = TRUE;
    Renderer::SetMaterial(material);

    // 使用しない骨は単位行列にしてから、現在の本体または破片の姿勢をGPUへ送る。
    std::array<Matrix, MaxBones> matrices;
    matrices.fill(Matrix::Identity);
    const auto& pose = broken ? m_debris.GetMatrices() : m_rig.GetMatrices();
    for (size_t index = 0; index < pose.size(); ++index) {
        matrices[index] = ToDirectX(pose[index]).Transpose();
    }

    auto context = Renderer::GetDeviceContext();
    context->UpdateSubresource(m_boneBuffer.Get(), 0, nullptr, matrices.data(), 0, 0);
    ID3D11Buffer* bones = m_boneBuffer.Get();
    context->VSSetConstantBuffers(BoneBufferSlot, 1, &bones);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->IASetInputLayout(m_layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_albedo.SetGPU();

    if (broken) {
        m_debrisVertices.SetGPU();
    } else {
        m_vertices.SetGPU();
    }
    context->Draw(m_vertexCount, 0);

    // 次に別のモデルを描くときへ、機体専用の骨バッファを持ち越さない。
    bones = nullptr;
    context->VSSetConstantBuffers(BoneBufferSlot, 1, &bones);
}

void PlayerRobotModel::Uninit() {
    // CPU側の姿勢と、GPU側の描画資源をまとめて解放する。
    m_ready = false;
    m_ikReady = false;
    m_spiderIK = RobotSpiderIK{};
    m_vertexCount = 0;
    m_rig = RobotAnimationRig{};
    m_debris.Reset();
    m_vertices = VertexBuffer<SkinnedVertex>{};
    m_debrisVertices = VertexBuffer<SkinnedVertex>{};
    m_albedo = Texture{};
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_layout.Reset();
    m_boneBuffer.Reset();
}
