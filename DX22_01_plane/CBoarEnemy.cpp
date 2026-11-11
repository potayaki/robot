/// @file CBoarEnemy.cpp
/// @brief 突進型の敵Boarと、その死亡演出。

#include "CBoarEnemy.h"
#include "Game.h"
#include "CPlayer.h"
#include "Ground.h"
#include "CBossBattle.h"
#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace DirectX::SimpleMath;

namespace {
    // 死亡演出はEnemyTargetから切り離す。ミサイル・誘導弾・砲台・ミニマップが
    // 倒れた個体を敵として選ばず、共通のDefeatで報酬も一度だけ渡せる。
    class BoarDeath : public Object {
        /// このオブジェクトの見た目を描画するモデル。
        std::unique_ptr<BoarModel> m_model;
        /// 現在の動作または演出を開始してからの経過秒数。
        float m_elapsed = 0;
        /// 水平方向の回転角（ラジアン）。
        float m_yaw = 0;
        /// モデルの拡大縮小倍率。
        float m_scale = 8;

    public:
        void Init() override {}

        void Start(std::unique_ptr<BoarModel> model, Vector3 feet, float yaw, float scale) {
            m_model = std::move(model);
            m_position = feet;
            m_yaw = yaw;
            m_scale = scale;
            m_model->Update(BoarMotion::Death, 0);
        }

        void Update() override {
            const float dt = GameTime::DeltaSeconds();
            m_elapsed += dt;
            if (m_model) {
                m_model->Update(BoarMotion::Death, dt);
            }
            // 約1.21秒の死亡動作の後、最後の姿勢を少し残してから取り除く。
            if (m_elapsed >= 2.0f) {
                Destroy();
            }
            for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
                if (battle->IsBattleActive()) {
                    Destroy();
                }
            }
        }

        void Draw(Camera* camera) override {
            if (m_model) {
                m_model->Draw(camera, m_position, m_yaw, m_scale);
            }
        }

        void Uninit() override {
            m_model.reset();
        }
    };
} // namespace

void CBoarEnemy::Init() {
    m_model = std::make_unique<BoarModel>();
    if (!m_model->Init()) {
        Destroy(); // 読み込みに失敗した透明な敵を、当たり判定だけ残して出現させない。
        return;
    }
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) {
        m_groundY = grounds[0]->GetPosition().y;
    }
    m_position.y = m_groundY + m_collisionRadius;
}

void CBoarEnemy::Update() {
    if (IsDead() || !m_model) {
        return;
    }
    const float dt = GameTime::DeltaSeconds();
    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) {
        m_groundY = grounds[0]->GetPosition().y;
    }
    m_position.y = m_groundY + m_collisionRadius;

    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) {
        m_model->Update(BoarMotion::Idle, dt);
        return;
    }
    auto player = players[0];
    const Vector3 previousPosition = m_position;
    const auto movement = m_charge.Update(dt, m_position, player->GetPosition(), m_chargeStartRange);
    const auto direction = m_charge.Direction();
    m_rotation.y = std::atan2(direction.x, direction.z);

    if (movement.charged && !m_charge.HasHit()) {
        // 高速移動では移動後の点だけを調べるとすり抜けるため、通った区間全体で判定する。
        const auto target = player->GetCollisionSphere();
        const float radius = m_collisionRadius + target.radius;
        const Collision::Segment segment{movement.start, movement.end};
        if (Collision::DistanceSquaredPointToSegment(target.center, segment) <= radius * radius) {
            player->TakeDamage(m_chargeDamage);
            m_charge.MarkHit(); // 同じ突進で何フレームも連続ダメージを与えない。
        }
    }

    BoarMotion motion = BoarMotion::Run;
    switch (m_charge.GetState()) {
        case BoarCharge::State::Chase:
            motion = (m_position - previousPosition).LengthSquared() > 0.000001f ? BoarMotion::Run
                                                                                 : BoarMotion::Idle;
            break;
        case BoarCharge::State::Windup:
            motion = BoarMotion::Windup;
            break;
        case BoarCharge::State::Charging:
            motion = BoarMotion::Charge;
            break;
        case BoarCharge::State::Recovery:
            motion = BoarMotion::Recover;
            break;
    }
    m_model->Update(motion, dt, m_charge.Progress());
}

void CBoarEnemy::Draw(Camera* camera) {
    if (m_model && !IsDead()) {
        m_model->Draw(camera, Vector3(m_position.x, m_groundY, m_position.z), m_rotation.y, m_modelScale);
    }
}

void CBoarEnemy::DrawTelegraph(Camera* camera) {
    if (IsDead() || !m_model || m_charge.GetState() != BoarCharge::State::Windup) {
        return;
    }
    m_model->DrawTelegraph(camera, m_charge.PreviewStart(), m_charge.PreviewEnd(), m_groundY,
                           m_collisionRadius, m_charge.Progress());
}

void CBoarEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    m_hp -= damage;
    if (m_hp > 0) {
        return;
    }

    if (m_model) {
        auto corpse = Game::GetInstance()->AddObject<BoarDeath>();
        corpse->Start(std::move(m_model), Vector3(m_position.x, m_groundY, m_position.z), m_rotation.y,
                      m_modelScale);
    }
    Defeat(m_experience, m_presentChance);
}

void CBoarEnemy::Uninit() {
    m_model.reset();
}

#ifdef _DEBUG
void CBoarEnemy::DrawDebugUI() {
    const auto text = [](const char8_t* value) { return reinterpret_cast<const char*>(value); };
    if (!ImGui::CollapsingHeader(text(u8"Boarの確認"))) {
        return;
    }
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty()) {
        return;
    }
    const auto player = players[0];
    bool bossActive = false;
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) {
        bossActive |= battle->IsBattleActive();
    }

    ImGui::BeginDisabled(bossActive || player->GetHp() <= 0);
    if (ImGui::Button(text(u8"Boarを正面に出す"))) {
        auto camera = Game::GetInstance()->GetCamera();
        camera->SetCamera();
        const Matrix inverseView = camera->GetViewMatrix().Invert();
        Vector3 forward = Vector3::TransformNormal(Vector3(0, 0, 1), inverseView);
        forward.y = 0;
        if (forward.LengthSquared() < 0.001f) {
            forward = Vector3(0, 0, 1);
        }
        forward.Normalize();
        auto boar = Game::GetInstance()->AddObject<CBoarEnemy>();
        Vector3 position = player->GetPosition() + forward * 150.0f;
        position.y = boar->m_groundY + boar->m_collisionRadius;
        boar->SetPosition(position);
    }
    ImGui::EndDisabled();

    CBoarEnemy* nearest = nullptr;
    float nearestDistance = 1e20f;
    for (auto boar : Game::GetInstance()->GetObjects<CBoarEnemy>()) {
        const float distance = (boar->GetPosition() - player->GetPosition()).LengthSquared();
        if (!boar->IsDead() && distance < nearestDistance) {
            nearest = boar;
            nearestDistance = distance;
        }
    }
    if (!nearest) {
        ImGui::TextUnformatted(text(u8"Boarがいません。上のボタンから出現させられます。"));
        return;
    }
    const char8_t* states[] = {u8"接近", u8"予備動作", u8"突進", u8"硬直"};
    ImGui::Text(text(u8"一番近いBoar: HP %d / %s"), nearest->m_hp,
                text(states[static_cast<int>(nearest->m_charge.GetState())]));
    ImGui::SliderFloat(text(u8"突進開始距離"), &nearest->m_chargeStartRange, 30, 400, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"予備動作（秒）"), &nearest->m_charge.m_windupSeconds, 0.3f, 3, "%.2f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"突進速度"), &nearest->m_charge.m_chargeSpeed, 80, 400, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(text(u8"突進距離"), &nearest->m_charge.m_chargeDistance, 60, 500, "%.0f",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextWrapped(
        "%s", text(u8"調整は一番近いBoarが対象です。予備動作の開始時に突進方向・速度・距離が確定します。"));
    if (ImGui::Button(text(u8"Boarに1ダメージ"))) {
        int damage = 1;
        nearest->OnHit(damage);
    }
    ImGui::SameLine();
    if (ImGui::Button(text(u8"Boarの死亡を確認"))) {
        int damage = 10000;
        nearest->OnHit(damage);
    }
}
#endif
