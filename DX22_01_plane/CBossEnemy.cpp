#include "CBossEnemy.h"
#include "CBossBattle.h"
#include "CPlayer.h"
#include "Game.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CBossEnemy::Init()
{
    // まずは既存スライムの3.5倍の大きさを使う。専用モデルへの差し替えはこの箇所で行う。
    m_body = std::make_unique<TestModel>();
    m_body->Init();
    m_body->Load("assets/model/slime/Slime.fbx", "assets/model/slime",
                 "assets/model/slime/albedo.tga", true);
    m_body->SetScale(0.7f);
}

void CBossEnemy::Configure(int hp, const Vector3& position)
{
    m_maxHp = (std::max)(1, hp);
    m_hp = m_maxHp;
    m_Position = position;
    UpdateModel(); // 出現直後の描画でも原点に一瞬表示されないようにする。
}

void CBossEnemy::Update()
{
    if (IsDead()) return;
    const auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) return;

    CPlayer* player = players[0];
    const float dt = GameTime::DeltaSeconds();
    GameTime::CountDown(m_contactCooldown, dt);
    GameTime::CountDown(m_phaseRemaining, dt);

    Vector3 direction = player->GetPosition() - m_Position;
    direction.y = 0;
    if (direction.LengthSquared() > 0.0001f) direction.Normalize();
    else direction = m_chargeDirection;

    if (m_phaseRemaining <= 0)
    {
        switch (m_phase)
        {
        case Phase::Approach:
            // 予備動作の開始時点で方向を固定するため、横に避ける余地がある。
            m_phase = Phase::Windup;
            m_phaseRemaining = 1.0f;
            m_chargeDirection = direction;
            m_Rotation.y = std::atan2(direction.x, direction.z);
            break;
        case Phase::Windup:
            m_phase = Phase::Charge;
            m_phaseRemaining = 0.85f;
            break;
        case Phase::Charge:
            m_phase = Phase::Recover;
            m_phaseRemaining = 1.0f;
            break;
        case Phase::Recover:
            m_phase = Phase::Approach;
            m_phaseRemaining = 2.0f;
            break;
        }
    }

    const Vector3 oldPosition = m_Position;
    if (m_phase == Phase::Approach)
    {
        // 通常の追跡では重なり続けない距離で止まる。突進だけはプレイヤーを通り越す。
        Vector3 horizontal = player->GetPosition() - m_Position;
        horizontal.y = 0;
        const float travel = (std::min)(22.0f * dt, (std::max)(0.0f, horizontal.Length() - 37.0f));
        m_Position += direction * travel;
        m_Rotation.y = std::atan2(direction.x, direction.z);
    }
    else if (m_phase == Phase::Charge)
    {
        m_Position += m_chargeDirection * (150.0f * dt);
    }

    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>())
    {
        m_Position = battle->ConstrainPosition(m_Position, GetCollisionSphere().radius);
    }

    // 突進の移動区間で判定し、低FPSでもプレイヤーを通り抜けて当たり損ねないようにする。
    const float hitRadius = GetCollisionSphere().radius + player->GetCollisionSphere().radius;
    const Collision::Segment movement{oldPosition, m_Position};
    if (m_contactCooldown <= 0 &&
        Collision::DistanceSquaredPointToSegment(player->GetPosition(), movement) <= hitRadius * hitRadius)
    {
        player->TakeDamage(m_phase == Phase::Charge ? 15 : 8);
        m_contactCooldown = 0.8f;
    }

    UpdateModel();
}

void CBossEnemy::UpdateModel()
{
    if (!m_body) return;
    // 通常スライムと同じ底面補正を3.5倍し、見た目と当たり判定を地面に合わせる。
    m_body->SetPosition(m_Position.x, m_Position.y - 35.0f + 1.75f, m_Position.z);
    m_body->SetRotation(Vector3(-30.0f, m_Rotation.y + DirectX::XM_PI, 0));

    // ため動作は縦に縮める。判定半径は変えず、動作を読みやすくする。
    const bool windingUp = m_phase == Phase::Windup;
    m_body->SetScale(Vector3(windingUp ? 0.78f : 0.7f, windingUp ? 0.56f : 0.7f, 0.7f));
}

void CBossEnemy::OnHit(int& damage)
{
    if (IsDead() || damage <= 0) return;
    m_hp = (std::max)(0, m_hp - damage);
    if (m_hp > 0) return;

    Defeat(15, 100); // 経験値15、ドロップ抽選100%。箱の同時存在上限は通常の敵と共通。
    // レベルアップでゲームが停止する場合でも、撃破した瞬間にバリアを解除する。
    for (auto battle : Game::GetInstance()->GetObjects<CBossBattle>()) battle->OnBossDefeated();
}

void CBossEnemy::Draw(Camera* cam)
{
    if (m_body && !IsDead()) m_body->Draw(cam);
}

void CBossEnemy::Uninit()
{
    if (m_body) m_body->Uninit();
    m_body.reset();
}
