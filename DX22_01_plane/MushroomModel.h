#pragma once
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "MeshRenderer.h"
#include "Material.h"
#include <memory>

class MushroomModel
{
public:
    bool Init();
    void Draw(Camera* camera, DirectX::SimpleMath::Vector3 feet, float yaw, bool red);
private:
    MeshRenderer m_mesh;
    Shader m_normalShader, m_redShader;
    std::vector<SUBSET> m_subsets;
    std::vector<std::unique_ptr<Material>> m_materials;
    Texture m_texture;
};
