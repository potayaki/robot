/// @brief タイトル台座の周囲から襲いかかる敵フィギュア。
#pragma once
#include "MeshRenderer.h"
#include "Shader.h"
#include "Texture.h"
#include "TitleEnemyGuide.h"
#include "TitleEnemyCamera.h"
#include <array>
#include <functional>

class TitleEnemyFigures {
public:
    bool Init(const std::function<float(float, float)>& groundHeight);
    const TitleEnemyCamera::Target* GetTarget(TitleEnemyKind kind) const;
    // タイトルのライティング、カメラ、深度設定を共有する。
    void Draw(const DirectX::SimpleMath::Matrix& root, const TitleEnemyCamera& camera,
              TitleEnemyKind selected, bool shadowPass = false);

private:
    struct Figure {
        MeshRenderer renderer;
        Texture texture;
        bool ready = false;
    };

    struct Placement {
        TitleEnemyKind kind;
        DirectX::SimpleMath::Matrix local;
        TitleEnemyCamera::Target target;
        float radius = 0;
    };

    std::array<Figure, static_cast<size_t>(TitleEnemyKind::Count)> m_figures{};
    std::array<Placement, 6> m_placements{};
    Shader m_portraitShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_portraitLight;
};
