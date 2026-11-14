#ifdef _DEBUG
#include "CStagePrimitive.h"
#include "Game.h"
#include "CPlayer.h"
#include "Ground.h"
#include "imgui.h"
#include <cmath>
#include <cstdio>

using namespace DirectX::SimpleMath;

void CStagePrimitive::DrawDebugUI() {
    static std::uint64_t selectedId = 0;
    static std::uint64_t textureEditorId = 0;
    static int createShape = static_cast<int>(PrimitiveMesh::Shape::Box);
    static char texturePath[1024] = "assets/texture/field.jpg";
    constexpr const char* names = "Triangle\0Quad\0Box\0Sphere\0Cone\0Cylinder\0";
    ImGui::SetNextWindowSize(ImVec2(450, 580), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stage Primitives")) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Create stage visuals, apply textures, then copy their placement code.");
    ImGui::TextWrapped("Use Skill Debug > Pause for the free camera. These parts have no collision.");
    ImGui::Combo("New shape", &createShape, names);
    if (ImGui::Button("Create in front of player")) {
        auto part = Game::GetInstance()->AddObject<CStagePrimitive>();
        const auto shape = static_cast<PrimitiveMesh::Shape>(createShape);
        part->SetShape(shape);
        part->SetScale(40, PrimitiveMesh::IsFlat(shape) ? 1.0f : 40.0f, 40);
        const auto players = Game::GetInstance()->GetObjects<CPlayer>();
        const auto grounds = Game::GetInstance()->GetObjects<Ground>();
        const float groundY = grounds.empty() ? 0 : grounds[0]->GetPosition().y;
        const float yaw = Game::GetInstance()->GetCamera()->GetCameraDirection();
        Vector3 position = players.empty() ? Vector3::Zero : players[0]->GetPosition();
        position += Vector3(-std::sin(yaw), 0, -std::cos(yaw)) * 100.0f;
        position.y = groundY + (PrimitiveMesh::IsFlat(shape) ? 0.2f : 20.0f);
        part->SetPosition(position);
        selectedId = part->m_id;
    }

    auto parts = Game::GetInstance()->GetObjects<CStagePrimitive>();
    CStagePrimitive* selected = nullptr;
    ImGui::BeginChild("Parts", ImVec2(0, 110), true);
    for (auto part : parts) {
        if (part->IsDead()) {
            continue;
        }
        char label[80];
        std::snprintf(label, sizeof(label), "%s #%llu", PrimitiveMesh::Name(part->m_shape),
                      static_cast<unsigned long long>(part->m_id));
        if (ImGui::Selectable(label, part->m_id == selectedId)) {
            selectedId = part->m_id;
        }
    }
    ImGui::EndChild();
    for (auto part : parts) {
        if (!part->IsDead() && part->m_id == selectedId) {
            selected = part;
        }
    }

    if (selected) {
        auto& part = *selected;
        int shape = static_cast<int>(part.m_shape);
        if (ImGui::Combo("Shape", &shape, names)) {
            part.SetShape(static_cast<PrimitiveMesh::Shape>(shape), part.m_slices, part.m_stacks);
        }
        int slices = part.m_slices, stacks = part.m_stacks;
        if (part.m_shape == PrimitiveMesh::Shape::Sphere || part.m_shape == PrimitiveMesh::Shape::Cone ||
            part.m_shape == PrimitiveMesh::Shape::Cylinder) {
            bool changed = ImGui::SliderInt("Slices", &slices, 3, 128, "%d", ImGuiSliderFlags_AlwaysClamp);
            if (part.m_shape == PrimitiveMesh::Shape::Sphere) {
                changed |= ImGui::SliderInt("Stacks", &stacks, 2, 64, "%d", ImGuiSliderFlags_AlwaysClamp);
            }
            if (changed) {
                part.SetShape(part.m_shape, slices, stacks);
            }
        }

        ImGui::DragFloat3("Position", &part.m_position.x, 0.5f);
        ImGui::DragFloat3("Size (XYZ)", &part.m_scale.x, 0.5f, 0.01f, 10000.0f, "%.2f",
                          ImGuiSliderFlags_AlwaysClamp);
        float rotation[] = {DirectX::XMConvertToDegrees(part.m_rotation.x),
                            DirectX::XMConvertToDegrees(part.m_rotation.y),
                            DirectX::XMConvertToDegrees(part.m_rotation.z)};
        if (ImGui::DragFloat3("Rotation (degrees)", rotation, 1.0f)) {
            part.SetRotationDegrees(rotation[0], rotation[1], rotation[2]);
        }
        ImGui::ColorEdit3("Tint", &part.m_tint.x);
        ImGui::Checkbox("Double sided", &part.m_doubleSided);
        ImGui::Separator();

        if (textureEditorId != selectedId) {
            std::snprintf(texturePath, sizeof(texturePath), "%s",
                          part.m_texturePath.empty() ? "assets/texture/field.jpg"
                                                     : part.m_texturePath.c_str());
            textureEditorId = selectedId;
        }
        ImGui::InputText("Texture path", texturePath, sizeof(texturePath));
        if (ImGui::Button("Apply texture")) {
            part.SetTexture(texturePath);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove texture")) {
            part.ClearTexture();
        }
        if (!part.m_textureError.empty()) {
            ImGui::TextWrapped("%s", part.m_textureError.c_str());
        }
        if (!part.m_texturePath.empty()) {
            ImGui::TextWrapped("Using: %s", part.m_texturePath.c_str());
        }
        ImGui::DragFloat2("UV repeat", &part.m_uvScale.x, 0.05f, -100, 100, "%.2f",
                          ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat2("UV offset", &part.m_uvOffset.x, 0.01f, -100, 100, "%.2f",
                          ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextWrapped(
            "PNG / JPG / BMP / TGA. Transparent PNG pixels are cut out. No texture = solid tint.");

        if (ImGui::Button("Duplicate")) {
            auto copy = Game::GetInstance()->AddObject<CStagePrimitive>();
            copy->SetShape(part.m_shape, part.m_slices, part.m_stacks);
            copy->SetPosition(part.m_position + Vector3(20, 0, 0));
            copy->SetRotation(part.m_rotation);
            copy->SetScale(part.m_scale);
            copy->SetTint(part.m_tint);
            copy->SetDoubleSided(part.m_doubleSided);
            copy->SetTextureTiling(part.m_uvScale.x, part.m_uvScale.y);
            copy->SetTextureOffset(part.m_uvOffset.x, part.m_uvOffset.y);
            copy->SetTexture(part.m_texturePath);
            selectedId = copy->m_id;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete selected")) {
            part.Destroy();
            selectedId = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy selected C++")) {
            ImGui::SetClipboardText(part.PlacementCode().c_str());
        }
    } else {
        ImGui::TextUnformatted("Create or select a primitive to edit it.");
    }

    if (ImGui::Button("Copy all placement C++")) {
        std::string code = "// Add #include \"CStagePrimitive.h\", then paste inside Stage1Scene::Init.\n";
        for (auto part : parts) {
            if (!part->IsDead()) {
                code += part->PlacementCode();
            }
        }
        ImGui::SetClipboardText(code.c_str());
    }
    ImGui::TextWrapped(
        "Edits last for this run. Paste the copied code into Stage1Scene::Init to keep the layout.");
    ImGui::End();
}
#endif
