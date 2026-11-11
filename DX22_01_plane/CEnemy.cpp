/// @file CEnemy.cpp
/// @brief 通常・高速・大型スライムの移動、接触攻撃、撃破報酬。

#include "CEnemy.h"
#include "Ground.h"
#include "Game.h"
#include "CPlayer.h"
#include "CPresentBox.h"
//ランダム
#include <cstdlib>
#include <cmath>

namespace {
    // 種類ごとの性能表。各値の役割を構造体の名前で示し、分岐ごとの重複を減らす。
    struct SlimeStats {
        /// 標準モデルと当たり判定に掛ける体格倍率。
        float size;
        /// 出現時の体力。
        int hp;
        /// 追跡する速度（ワールド単位/秒）。
        float speed;
        /// 接触時のダメージ量。
        int damage;
        /// 撃破時の経験値。
        int experience;
        /// プレゼントを落とす確率（%）。
        int presentChance;
    };

    constexpr SlimeStats NormalStats{1.0f, 3, 30.0f, 2, 1, 10};
    constexpr SlimeStats RunnerStats{0.7f, 2, 57.0f, 2, 2, 10};
    constexpr SlimeStats BruteStats{1.8f, 12, 18.0f, 6, 5, 25};

    constexpr float SlimeFacingOffset = DirectX::XM_PI; // モデルの正面はゲームの+Zと逆向き
    // FBXを0.2倍・X軸-30ラジアンにしたとき、モデルの最下点は原点より約0.5下にある。
    constexpr float SlimeModelBottomY = -0.5f;
} // namespace

CEnemy::CEnemy() {
    m_body = nullptr;
}

CEnemy::~CEnemy() {
    Uninit();
}

void CEnemy::Init() {
    m_body = new TestModel();
    m_body->Init();
    // FBX が参照する元の Albedo.png は無いため、同梱の albedo.tga を使う。
    m_body->Load("assets/model/slime/Slime.fbx", "assets/model/slime", "assets/model/slime/albedo.tga", true);
    m_body->SetScale(m_modelScale); // 敵の体
    m_rotation.x = m_modelPitch; // スライムの見た目に合わせた傾き
    m_body->SetRotation(
        DirectX::SimpleMath::Vector3(m_rotation.x, m_rotation.y + SlimeFacingOffset, m_rotation.z));
    //m_body->SetMaterial(DirectX::SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f)); // 白色のマテリアル
   
}

void CEnemy::Update() {
    if (IsDead()) {
        return;
    }
    const float deltaSeconds = GameTime::DeltaSeconds();
    // 攻撃待ちとダッシュの周期を、フレーム数ではなく秒で進める。
    m_moveTime = std::fmod(m_moveTime + deltaSeconds, m_dashPeriod);
    GameTime::AdvanceCooldown(m_attackTimer, deltaSeconds);
    //地面との当たり判定-----------------------------------------------------------------------
    std::vector<Ground*> grounds = Game::GetInstance()->GetObjects<Ground>();
    if (!grounds.empty()) { //groundsの中が空っぽかそうか１つ以上作られたか
        Ground* plane = grounds[0];
        float planeY = plane->GetPosition().y + GetCollisionSphere().radius; //床座標の取得 +　自身の半径

        if (m_position.y <= planeY) {
            m_position.y = planeY;
            Onland();
        } else {
            //空中状態
            m_isGrounded = false;
        }
    }

    //移動-----------------------------------------------------------
    std::vector<CPlayer*> player = Game::GetInstance()->GetObjects<CPlayer>();
    if (!player.empty() && player[0] != nullptr) {
        DirectX::SimpleMath::Vector3 PlayerPos = player[0]->GetPosition();

        DirectX::SimpleMath::Vector3 dir = PlayerPos - m_position;
        dir.y = 0.0f;

        float distance = dir.Length();

        if (dir.LengthSquared() > 0.0001f) {
            dir.Normalize();

            // 高速型は一定周期で短いダッシュを行う。
            const float dash =
                m_variant == SlimeVariant::Runner && m_moveTime < m_dashDuration ? m_dashMultiplier : 1.0f;
            m_position += dir * m_speed * dash * deltaSeconds;
            m_rotation.y = atan2f(dir.x, dir.z);
        }

        float hitRange = GetCollisionSphere().radius + player[0]->GetCollisionSphere().radius;

        if (distance < hitRange && m_attackTimer <= 0.0f) {
            // プレイヤーと敵が接触した場合の処理
            player[0]->TakeDamage(m_damage); // プレイヤーにダメージを与える
            m_attackTimer += m_contactInterval; // 接触攻撃は1秒間隔。
        }
    }

    // 当たり判定の球は中心を半径分だけ地面から上げているため、モデルの底面が地面に合う位置へ下げる。
    m_body->SetPosition(m_position.x, m_position.y - GetCollisionSphere().radius - SlimeModelBottomY * m_size,
                        m_position.z);
    m_body->SetRotation(
        DirectX::SimpleMath::Vector3(m_rotation.x, m_rotation.y + SlimeFacingOffset, m_rotation.z));
}

void CEnemy::SetVariant(SlimeVariant variant) {
    m_variant = variant;
    // 小型は追い込み役、大型は高耐久・高報酬の敵として調整する。
    const SlimeStats& stats = variant == SlimeVariant::Runner  ? RunnerStats
                              : variant == SlimeVariant::Brute ? BruteStats
                                                               : NormalStats;
    m_size = stats.size;
    m_hp = stats.hp;
    m_speed = stats.speed;
    m_damage = stats.damage;
    m_experience = stats.experience;
    m_dropPercent = stats.presentChance;
    if (m_body) {
        m_body->SetScale(m_modelScale * m_size);
    }
}

void CEnemy::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void CEnemy::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

void CEnemy::Onland() {
    m_isGrounded = true;
}

void CEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    // 共通の被弾処理ではダメージだけを扱い、爆発と画面揺れはミサイルの命中処理に任せる。
    m_hp -= damage; // 体力を減らす
    if (m_hp <= 0) {
        Defeat(m_experience, m_dropPercent);
    }
}
