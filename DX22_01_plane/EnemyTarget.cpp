#include "EnemyTarget.h"
#include "Game.h"
#include "CPlayer.h"
#include "Ground.h"
#include "CPresentBox.h"
#include <cstdlib>

void EnemyTarget::Defeat(int experience, int presentChance) {
    // 同じフレームの複数の弾・範囲攻撃でも、報酬を一度だけ確定する。
    if (IsDead()) return;
    Destroy();
    auto players = Game::GetInstance()->GetObjects<CPlayer>();
    if (players.empty() || players[0]->GetHp() <= 0) return;
    CPlayer* player = players[0];
    player->OnEnemyDefeated(experience);
    // 通常抽選に加え10体ごとに確定ドロップし、序盤でもプレゼントを試せるようにする。
    if (rand() % 100 >= presentChance && player->GetKills() % 10 != 0) return;
    auto grounds = Game::GetInstance()->GetObjects<Ground>();
    const float groundY = grounds.empty() ? -5.0f : grounds[0]->GetPosition().y;
    auto present = Game::GetInstance()->AddObject<CPresentBox>();
    // 空中の敵を倒した場合も、拾える地面の高さに報酬を配置する。
    present->SetPosition(m_Position.x, groundY + 10.0f, m_Position.z);
}
