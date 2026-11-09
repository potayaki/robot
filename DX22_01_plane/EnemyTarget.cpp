#include "EnemyTarget.h"
#include "Game.h"
#include "CPlayer.h"
#include "Ground.h"
#include "CPresentBox.h"
#include "CSpanner.h"
#include <algorithm>
#include <cstdlib>

void EnemyTarget::Defeat(int experience, int presentChance)
{
    // 敵の種類によらず共通の撃破処理。経験値とドロップ率は呼び出し側で指定する。
    // 同じフレームの複数の弾・範囲攻撃でも、報酬を一度だけ確定する。
    if (IsDead())
    {
        return;
    }
    Destroy();

    // 報酬を渡せるプレイヤーがいなければ、敵の消滅だけで処理を終える。
    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0)
    {
        return;
    }
    CPlayer* player = players[0];
    player->OnEnemyDefeated(experience);

    const auto grounds = Game::GetInstance()->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    const bool dropSpanner = rand() % 100 < std::clamp(CSpanner::m_spawnChancePercent, 0, 100);
    if (dropSpanner)
    {
        auto spanner = Game::GetInstance()->AddObject<CSpanner>();
        spanner->SetPosition(m_Position.x, groundY, m_Position.z);
    }

    // 通常抽選に加え10体ごとに確定ドロップし、序盤でもプレゼントを試せるようにする。
    if (rand() % 100 >= presentChance && player->GetKills() % 10 != 0)
    {
        return;
    }

    // 箱の個数制限はCPresentBox::Initが担当する。ここでは生成位置を決める。
    auto present = Game::GetInstance()->AddObject<CPresentBox>();

    // 空中の敵を倒した場合も、拾える地面の高さに報酬を配置する。
    present->SetPosition(m_Position.x + (dropSpanner ? 25.0f : 0.0f), groundY + 10.0f, m_Position.z);
}
