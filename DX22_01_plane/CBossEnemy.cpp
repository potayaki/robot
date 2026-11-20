/// @file CBossEnemy.cpp
/// @brief 種族ごとの攻撃と予告を共通のボス戦へ接続する。
#include "CBossEnemy.h"
#include "CBossBattle.h"
#include "CPlayer.h"
#include "Game.h"
#include <cmath>

using namespace DirectX::SimpleMath;

namespace {
    constexpr float SporeRadius = 55.0f;
    constexpr float SpiritRadius = 7.0f;
    constexpr float SpiritSpeed = 135.0f;
    constexpr float BoarSpeed = 280.0f;
    constexpr float BoarChargeSeconds = 1.1f;
    constexpr float BoarScale = 22.85f;
    constexpr float FlyScale = 0.46f;
    constexpr float MushroomScale = 36.0f / 14.0f;
    constexpr float GhostScale = 30.0f / 16.0f;
    constexpr int ContactDamage = 8;
    constexpr int ChargeDamage = 15;
    constexpr int SporeDamage = 12;
    constexpr int SpiritDamage = 12;
    constexpr int CircleSegments = 48;
    constexpr int EffectVertexCapacity = 3 * CircleSegments * 9;

    Vector3 HorizontalDirection(Vector3 from, Vector3 to) {
        Vector3 direction = to - from;
        direction.y = 0;
        if (direction.LengthSquared() < 0.0001f) {
            return Vector3(0, 0, 1);
        }
        direction.Normalize();
        return direction;
    }
} // namespace

void CBossEnemy::Init() {
    // 種類が確定するConfigureで、そのボスのモデルだけ読み込む。
    m_attack.Reset(m_kind);
}

void CBossEnemy::Configure(int hp, const Vector3& position, BossKind kind) {
    Uninit();
    m_kind = kind;
    m_maxHp = (std::max)(1, hp);
    m_hp = m_maxHp;
    m_position = position;
    m_groundY = position.y - BossHeight(kind);
    m_elapsed = m_damageCooldown = 0;
    m_attackHit = false;
    m_orbitSign = 1;
    m_attack.Reset(kind);
    switch (kind) {
        case BossKind::Slime:
            m_body = std::make_unique<TestModel>();
            m_body->Init();
            m_body->Load("assets/model/slime/Slime.fbx", "assets/model/slime",
                         "assets/model/slime/albedo.tga", true);
            break;
        case BossKind::Fly:
            m_body = std::make_unique<TestModel>();
            m_body->Init();
            m_body->Load("assets/model/drone/Drone.fbx", "assets/model/drone",
                         "assets/model/drone/albedo.tga", true);
            break;
        case BossKind::Boar:
            m_boar = std::make_unique<BoarModel>();
            if (!m_boar->Init()) {
                Destroy();
            }
            break;
        case BossKind::Mushroom:
            m_mushroom = std::make_unique<MushroomModel>();
            if (!m_mushroom->Init()) {
                Destroy();
            }
            break;
        case BossKind::Ghost:
            m_ghost = GhostModel::Acquire();
            if (!m_ghost) {
                Destroy();
            }
            break;
        default:
            Destroy();
            return;
    }
    if (kind == BossKind::Fly || kind == BossKind::Mushroom) {
        m_shader.Create("shader/unlitTextureVS.hlsl", "shader/unlitTexturePS.hlsl");
        if (m_white.Load("assets/texture/white.png")) {
            m_effectBuffer.Create(std::vector<VERTEX_3D>(EffectVertexCapacity));
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            desc.DepthClipEnable = TRUE;
            m_effectsReady = SUCCEEDED(Renderer::GetDevice()->CreateRasterizerState(
                &desc, m_effectRasterizer.ReleaseAndGetAddressOf()));
        }
    }
    UpdateModel(0);
}

Vector3 CBossEnemy::Constrain(const Vector3& position, float radius) const {
    Vector3 result = position;
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        result = battle->ConstrainPosition(result, radius);
    }
    return result;
}

void CBossEnemy::EnterPhase(CPlayer* player) {
    if (GetPhase() == Phase::Windup) {
        m_attackHit = false;
        m_attackStart = m_position;
        m_chargeDirection = HorizontalDirection(m_position, player->GetPosition());
        m_rotation.y = std::atan2(m_chargeDirection.x, m_chargeDirection.z);
        m_attackTarget = player->GetCollisionSphere().center;
        if (m_kind == BossKind::Fly) {
            m_attackTarget += m_chargeDirection * 70.0f;
            m_attackTarget.y = m_groundY + BossRadius(m_kind);
            m_attackTarget = Constrain(m_attackTarget, BossRadius(m_kind));
        } else if (m_kind == BossKind::Boar) {
            m_attackTarget = Constrain(m_attackStart + m_chargeDirection * (BoarSpeed * BoarChargeSeconds),
                                       BossRadius(m_kind));
        } else if (m_kind == BossKind::Mushroom) {
            const Vector3 side(m_chargeDirection.z, 0, -m_chargeDirection.x);
            for (size_t i = 0; i < m_spores.size(); ++i) {
                m_spores[i] =
                    Constrain(m_attackTarget + side * (static_cast<float>(i) - 1.0f) * 100.0f, SporeRadius);
                m_spores[i].y = m_groundY + 1.0f;
            }
        }
        if (m_kind == BossKind::Fly || m_kind == BossKind::Boar) {
            m_chargeDirection = HorizontalDirection(m_attackStart, m_attackTarget);
            m_rotation.y = std::atan2(m_chargeDirection.x, m_chargeDirection.z);
        }
    } else if (GetPhase() == Phase::Charge && m_kind == BossKind::Ghost) {
        Vector3 aim = m_attackTarget - m_position;
        if (aim.LengthSquared() < 0.0001f) {
            aim = m_chargeDirection;
        }
        aim.Normalize();
        for (int i = -2; i <= 2; ++i) {
            const Vector3 velocity =
                Vector3::TransformNormal(aim, Matrix::CreateRotationY(i * 0.22f)) * SpiritSpeed;
            m_shots.push_back({m_position, velocity});
        }
    } else if (GetPhase() == Phase::Recover) {
        m_orbitSign = -m_orbitSign;
    }
}

void CBossEnemy::Update() {
    if (IsDead()) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        m_shots.clear();
        return;
    }
    auto player = players[0];
    float remaining = GameTime::DeltaSeconds();
    // フェーズ境界をまたぐ時間を次の動作へ渡し、FPSで予告や突進時間が変わらないようにする。
    while (remaining > 0.000001f && player->GetHp() > 0) {
        const float step = m_attack.Consume(remaining);
        remaining -= step;
        m_elapsed += step;
        GameTime::CountDown(m_damageCooldown, step);
        UpdateMovement(player, step);
        UpdateShots(player, step);
        UpdateModel(step);
        if (m_attack.Advance()) {
            EnterPhase(player);
        }
    }
    // 境界直後の描画にも新しい予告姿勢を反映する。
    UpdateModel(0);
}

void CBossEnemy::UpdateMovement(CPlayer* player, float seconds) {
    const Phase phase = GetPhase();
    const Vector3 before = m_position;
    const Vector3 direction = HorizontalDirection(m_position, player->GetPosition());
    Vector3 offset = player->GetPosition() - m_position;
    offset.y = 0;
    const float distance = offset.Length();
    if (phase == Phase::Approach) {
        float speed = 22.0f;
        float stop = 37.0f;
        if (m_kind == BossKind::Boar) {
            speed = 55.0f;
            stop = 160.0f;
        }
        if (m_kind == BossKind::Mushroom) {
            speed = 28.0f;
            stop = 150.0f;
        }
        if (m_kind == BossKind::Fly || m_kind == BossKind::Ghost) {
            speed = 45.0f;
            stop = 180.0f;
        }
        m_position += direction * (std::min)(speed * seconds, (std::max)(0.0f, distance - stop));
        m_rotation.y = std::atan2(direction.x, direction.z);
        if (m_kind == BossKind::Fly || m_kind == BossKind::Ghost) {
            const Vector3 side(direction.z, 0, -direction.x);
            m_position += side * (m_orbitSign * 30.0f * seconds);
        }
    } else if (phase == Phase::Charge) {
        if (m_kind == BossKind::Slime) {
            m_position += m_chargeDirection * (150.0f * seconds);
        }
        if (m_kind == BossKind::Fly || m_kind == BossKind::Boar) {
            m_position = Vector3::Lerp(m_attackStart, m_attackTarget, m_attack.Progress());
        }
    } else if (phase == Phase::Recover && m_kind == BossKind::Ghost) {
        // 発射後に横へ退避する。移動中も照準・被弾の対象になる。
        m_position += Vector3(direction.z, 0, -direction.x) * (m_orbitSign * 90.0f * seconds);
    }
    if (m_kind == BossKind::Fly && (phase == Phase::Approach || phase == Phase::Recover)) {
        const float hover = m_groundY + BossHeight(m_kind) + std::sin(m_elapsed * 2.5f) * 7.0f;
        m_position.y += std::clamp(hover - m_position.y, -65.0f * seconds, 65.0f * seconds);
    } else if (m_kind == BossKind::Ghost) {
        m_position.y = m_groundY + BossHeight(m_kind) + std::sin(m_elapsed * 2.5f) * 5.0f;
    }
    m_position = Constrain(m_position, BossRadius(m_kind));
    const auto target = player->GetCollisionSphere();
    const float hitRadius = BossRadius(m_kind) + target.radius;
    const bool striking = phase == Phase::Charge && (m_kind == BossKind::Fly || m_kind == BossKind::Boar);
    if (m_damageCooldown <= 0 && (!striking || !m_attackHit) &&
        Collision::DistanceSquaredPointToSegment(target.center, {before, m_position}) <=
            hitRadius * hitRadius) {
        const int damage = phase == Phase::Charge ? ChargeDamage : ContactDamage;
        player->TakeDamage(SupportedOutgoingDamage(damage));
        m_damageCooldown = 0.8f;
        m_attackHit = striking;
    }
    if (m_kind == BossKind::Mushroom && phase == Phase::Charge && m_damageCooldown <= 0) {
        for (const auto& center : m_spores) {
            Vector3 horizontal = target.center - center;
            horizontal.y = 0;
            const float reach = SporeRadius + target.radius;
            if (horizontal.LengthSquared() <= reach * reach &&
                target.center.y - target.radius <= m_groundY + 35.0f) {
                player->TakeDamage(SupportedOutgoingDamage(SporeDamage));
                m_damageCooldown = 0.85f;
                break;
            }
        }
    }
}

void CBossEnemy::UpdateShots(CPlayer* player, float seconds) {
    const auto target = player->GetCollisionSphere();
    for (auto& shot : m_shots) {
        const Vector3 before = shot.position;
        shot.position += shot.velocity * (std::min)(seconds, shot.remaining);
        shot.remaining -= seconds;
        const float radius = SpiritRadius + target.radius;
        if (Collision::DistanceSquaredPointToSegment(target.center, {before, shot.position}) <=
            radius * radius) {
            player->TakeDamage(SupportedOutgoingDamage(SpiritDamage));
            shot.remaining = 0;
        }
        if (shot.position.y <= m_groundY + SpiritRadius ||
            (Constrain(shot.position, SpiritRadius) - shot.position).LengthSquared() > 0.01f) {
            shot.remaining = 0;
        }
    }
    std::erase_if(m_shots, [](const SpiritShot& shot) { return shot.remaining <= 0; });
}

void CBossEnemy::UpdateModel(float seconds) {
    if (m_body && m_kind == BossKind::Slime) {
        m_body->SetPosition(m_position.x, m_position.y - BossRadius(m_kind) + 1.75f, m_position.z);
        m_body->SetRotation(Vector3(-30.0f, m_rotation.y + DirectX::XM_PI, 0));
        const bool windup = GetPhase() == Phase::Windup;
        m_body->SetScale(Vector3(windup ? 0.78f : 0.7f, windup ? 0.56f : 0.7f, 0.7f));
    } else if (m_body && m_kind == BossKind::Fly) {
        const Vector3 center(0, -2.9281f, -66.2454f);
        const Matrix rotation = Matrix::CreateRotationY(m_rotation.y);
        m_body->SetScale(FlyScale);
        m_body->SetRotation(Vector3(0, m_rotation.y, 0));
        m_body->SetPosition(m_position - Vector3::TransformNormal(center * FlyScale, rotation));
    }
    if (m_boar) {
        constexpr BoarMotion motions[] = {BoarMotion::Run, BoarMotion::Windup, BoarMotion::Charge,
                                          BoarMotion::Recover};
        m_boar->Update(motions[static_cast<int>(GetPhase())], seconds, m_attack.Progress());
    }
}

void CBossEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    m_hp = (std::max)(0, m_hp - SupportedIncomingDamage(damage));
    if (m_hp > 0) {
        return;
    }
    m_shots.clear();
    Defeat(15, 100);
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        battle->OnBossDefeated(m_kind);
    }
}

void CBossEnemy::Draw(Camera* camera) {
    if (IsDead()) {
        return;
    }
    if (m_body) {
        m_body->Draw(camera);
    }
    if (m_boar) {
        m_boar->Draw(camera, Vector3(m_position.x, m_groundY, m_position.z), m_rotation.y, BoarScale);
    }
    if (m_mushroom) {
        const bool flash = GetPhase() == Phase::Windup && static_cast<int>(m_elapsed * 10) % 2 == 0;
        m_mushroom->Draw(camera, Vector3(m_position.x, m_groundY, m_position.z), m_rotation.y, flash,
                         MushroomScale);
    }
    if (m_ghost) {
        m_ghost->Draw(camera, m_position, m_rotation.y, GhostScale);
    }
}

const char8_t* CBossEnemy::GetAttackNotice() const {
    if (GetPhase() == Phase::Recover) {
        if (m_kind == BossKind::Ghost) {
            return u8"霊弾を避けながら退避中のボスを追撃しよう";
        }
        if (m_kind == BossKind::Fly) {
            return u8"ボスが上昇中！ 攻撃のチャンス";
        }
        return u8"ボスが休んでいる！ 攻撃のチャンス";
    }
    if (GetPhase() == Phase::Approach) {
        return u8"ボスの動きを見ながら攻撃しよう";
    }
    switch (m_kind) {
        case BossKind::Fly:
            return GetPhase() == Phase::Windup ? u8"急降下の予告！ オレンジの進路から離れよう"
                                               : u8"急降下中！ 横へ避けよう";
        case BossKind::Boar:
            return GetPhase() == Phase::Windup ? u8"2連続突進！ 赤い進路から離れよう"
                                               : u8"突進中！ 次の予告にも注意";
        case BossKind::Mushroom:
            return GetPhase() == Phase::Windup ? u8"胞子の予告！ 紫の円から離れよう"
                                               : u8"胞子が発生中！ 円の外から攻撃しよう";
        case BossKind::Ghost:
            return GetPhase() == Phase::Windup ? u8"霊弾の予告！ 光の方向から離れよう"
                                               : u8"扇状の霊弾！ 弾の間を抜けよう";
        default:
            return GetPhase() == Phase::Windup ? u8"突進の予備動作！ 横へ避けよう" : u8"突進中！";
    }
}

void CBossEnemy::DrawEffects(Camera* camera) {
    if (IsDead() || !GetActive()) {
        return;
    }
    const bool warning = GetPhase() == Phase::Windup;
    if (m_boar && warning) {
        m_boar->DrawTelegraph(camera, m_attackStart, m_attackTarget, m_groundY, BossRadius(m_kind),
                              m_attack.Progress());
    }
    if (m_ghost) {
        if (warning) {
            m_ghost->DrawBeam(camera, m_position, m_attackTarget, 3, Color(0.7f, 0.3f, 1, 1));
        }
        for (const auto& shot : m_shots) {
            m_ghost->DrawBeam(camera, shot.position - Vector3(0, 2, 0), shot.position + Vector3(0, 2, 0),
                              SpiritRadius * 2, Color(0.35f, 0.85f, 1, 1), true);
        }
    }
    if (!m_effectsReady || (!warning && GetPhase() != Phase::Charge)) {
        return;
    }
    std::vector<VERTEX_3D> vertices;
    vertices.reserve(EffectVertexCapacity);
    const auto ring = [&](Vector3 center, float radius, Color color) {
        center.y = m_groundY + 0.7f;
        const auto vertex = [&](Vector3 position, Color tint) {
            VERTEX_3D result{};
            result.position = position;
            result.color = tint;
            result.uv = Vector2(0.5f, 0.5f);
            vertices.push_back(result);
        };
        for (int i = 0; i < CircleSegments; ++i) {
            const float a = DirectX::XM_2PI * i / CircleSegments;
            const float b = DirectX::XM_2PI * (i + 1) / CircleSegments;
            const Vector3 da(std::cos(a), 0, std::sin(a));
            const Vector3 db(std::cos(b), 0, std::sin(b));
            const Vector3 corners[] = {center + da * (radius - 2), center + da * radius,
                                       center + db * (radius - 2), center + db * (radius - 2),
                                       center + da * radius,       center + db * radius};
            for (const auto& point : corners) {
                vertex(point, color);
            }
            const Color fill(color.x, color.y, color.z, warning ? 0.08f : 0.22f);
            vertex(center, fill);
            vertex(center + da * radius, fill);
            vertex(center + db * radius, fill);
        }
    };
    if (m_kind == BossKind::Mushroom) {
        for (const auto& spore : m_spores) {
            ring(spore, SporeRadius, Color(0.85f, 0.25f, 1, 0.85f));
        }
    } else if (m_kind == BossKind::Fly) {
        ring(m_attackTarget, BossRadius(m_kind), Color(1, 0.35f, 0.1f, 0.9f));
        Vector3 start = m_attackStart;
        Vector3 end = m_attackTarget;
        start.y = end.y = m_groundY + 0.8f;
        const Vector3 direction = HorizontalDirection(start, end);
        const Vector3 side = Vector3(direction.z, 0, -direction.x) * BossRadius(m_kind);
        const Vector3 corners[] = {start - side, start + side, end - side,
                                   end - side,   start + side, end + side};
        for (const auto& point : corners) {
            VERTEX_3D vertex{};
            vertex.position = point;
            vertex.uv = Vector2(0.5f, 0.5f);
            vertex.color = Color(1, 0.35f, 0.1f, 0.18f);
            vertices.push_back(vertex);
        }
    }
    if (vertices.empty()) {
        return;
    }
    m_effectBuffer.Modify(vertices);
    auto context = Renderer::GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;
    float factors[4]{};
    UINT mask = 0, stencil = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context->OMGetBlendState(blend.GetAddressOf(), factors, &mask);
    context->OMGetDepthStencilState(depth.GetAddressOf(), &stencil);
    context->RSGetState(rasterizer.GetAddressOf());
    context->IAGetPrimitiveTopology(&topology);
    camera->SetCamera();
    Matrix world = Matrix::Identity;
    Renderer::SetWorldMatrix(&world);
    Renderer::SetUV(0, 0, 1, 1);
    Renderer::SetBlendState(BS_ADDITIVE);
    Renderer::SetDepthEnable(false);
    context->RSSetState(m_effectRasterizer.Get());
    m_shader.SetGPU();
    m_white.SetGPU();
    m_effectBuffer.SetGPU();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(static_cast<UINT>(vertices.size()), 0);
    context->OMSetBlendState(blend.Get(), factors, mask);
    context->OMSetDepthStencilState(depth.Get(), stencil);
    context->RSSetState(rasterizer.Get());
    context->IASetPrimitiveTopology(topology);
}

void CBossEnemy::Uninit() {
    m_shots.clear();
    m_body.reset();
    m_boar.reset();
    m_mushroom.reset();
    m_ghost.reset();
    m_effectsReady = false;
    m_effectRasterizer.Reset();
    m_effectBuffer = VertexBuffer<VERTEX_3D>{};
    m_white = Texture{};
    m_shader = Shader{};
}
