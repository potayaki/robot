/// @file CEnemy.cpp
/// @brief 通常・高速・大型スライムの移動、接触攻撃、撃破報酬。

#include "CEnemy.h"
#include "Ground.h"
#include "Game.h"
#include "CPlayer.h"
#include "CPresentBox.h"
#include "CForestStage.h"
#include "EnemyCrowd.h"
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
        /// 経験値オーブ10粒を落とす抽選の基礎重み。
        int orbDropWeight;
        /// プレゼントを落とす確率（%）。
        int presentChance;
    };

    constexpr SlimeStats NormalStats{1.0f, 3, 30.0f, 2, 1, 1};
    constexpr SlimeStats RunnerStats{0.7f, 2, 57.0f, 2, 2, 1};
    constexpr SlimeStats BruteStats{1.8f, 12, 18.0f, 6, 5, 4};

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
    //移動-----------------------------------------------------------
    const auto previousPosition = m_position;
    std::vector<CPlayer*> player = Game::GetInstance()->GetObjects<CPlayer>();
    if (!player.empty() && player[0] != nullptr) {
        DirectX::SimpleMath::Vector3 PlayerPos = player[0]->GetPosition();

        DirectX::SimpleMath::Vector3 dir = PlayerPos - m_position;
        dir.y = 0.0f;

        if (dir.LengthSquared() > 0.0001f) {
            m_rotation.y = atan2f(dir.x, dir.z);
        }

        std::vector<EnemyCrowd::Neighbor> neighbors;
        const auto slimes = Game::GetInstance()->GetObjects<CEnemy>();
        neighbors.reserve(slimes.size());
        for (auto slime : slimes) {
            if (slime != this && !slime->IsDead() && slime->GetActive()) {
                const auto sphere = slime->GetCollisionSphere();
                neighbors.push_back({sphere.center, sphere.radius, reinterpret_cast<std::uintptr_t>(slime)});
            }
        }
        // 高速型のダッシュも含めて移動量を制限し、大型・小型の半径で互いを押し分ける。
        const float dash =
            m_variant == SlimeVariant::Runner && m_moveTime < m_dashDuration ? m_dashMultiplier : 1.0f;
        const float speed = m_speed * EnemyVariants::Get(GetRarity()).speedMultiplier * dash;
        m_position = EnemyCrowd::Move(m_position, PlayerPos, GetCollisionSphere().radius,
                                      player[0]->GetCollisionSphere().radius, speed, deltaSeconds, neighbors,
                                      reinterpret_cast<std::uintptr_t>(this));

        for (auto forest : Game::GetInstance()->GetObjects<CForestStage>()) {
            m_position = forest->ConstrainMovement(previousPosition, m_position, GetCollisionSphere().radius);
        }

        auto separation = PlayerPos - m_position;
        separation.y = 0;
        const float distance = separation.Length();

        float hitRange = GetCollisionSphere().radius + player[0]->GetCollisionSphere().radius;

        if (distance < hitRange && m_attackTimer <= 0.0f) {
            // プレイヤーと敵が接触した場合の処理
            player[0]->TakeDamage(SupportedOutgoingDamage(m_damage)); // 支援中の攻撃力を反映。
            m_attackTimer += m_contactInterval; // 接触攻撃は1秒間隔。
        }
    }
    if (const Ground* ground = Ground::Current()) {
        m_position.y = ground->HeightAt(m_position.x, m_position.z) + GetCollisionSphere().radius;
    }
}

void CEnemy::SetVariant(SlimeVariant variant) {
    m_variant = variant;
    // 小型は追い込み役、大型は高耐久・高報酬の敵として調整する。
    const SlimeStats& stats = variant == SlimeVariant::Runner  ? RunnerStats
                              : variant == SlimeVariant::Brute ? BruteStats
                                                               : NormalStats;
    m_size = stats.size;
    m_hp = EnemyVariants::Health(stats.hp, GetRarity());
    m_speed = stats.speed;
    m_damage = stats.damage;
    m_orbDropWeight = stats.orbDropWeight;
    m_dropPercent = stats.presentChance;
}

void CEnemy::SetRarity(EnemyRarity rarity) {
    EnemyTarget::SetRarity(rarity);
    // 体格・役割を保ち、再設定でもHP倍率を重ねない。
    SetVariant(m_variant);
}

void CEnemy::Draw(Camera* cam) {
    if (m_body) {
        // 生成直後はまだUpdateされていないため、モデルに保存した古い位置は使わない。
        // 当たり判定の中心からモデル底面を下げ、既存の接地位置を維持する。
        using namespace DirectX::SimpleMath;
        const auto hit = HitPose();
        const Vector3 feet = m_position - Vector3(0, GetCollisionSphere().radius, 0) + hit.offset;
        const float width = 1.0f + 0.12f * hit.recoil;
        const float height = 1.0f - 0.22f * hit.recoil;
        // 素材の傾きを補正してから足元を原点に押しつぶす。体格・向きに関わらず接地を保つ。
        const Matrix world =
            Matrix::CreateScale(m_modelScale * m_size) *
            Matrix::CreateFromYawPitchRoll(m_rotation.y + SlimeFacingOffset, m_rotation.x, m_rotation.z) *
            Matrix::CreateTranslation(0, -SlimeModelBottomY * m_size, 0) *
            Matrix::CreateScale(width, height, width) * Matrix::CreateTranslation(feet);
        m_body->DrawWorld(cam, world, EnemyVariants::Tint(EnemySpecies::Slime, GetRarity()), hit.flash);
    }
}

void CEnemy::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

void CEnemy::OnHit(int& damage) {
    if (IsDead() || damage <= 0) {
        return;
    }
    // 共通の被弾処理ではダメージだけを扱い、爆発と画面揺れはミサイルの命中処理に任せる。
    m_hp -= SupportedIncomingDamage(damage); // 攻撃元のダメージ値は変更しない。
    if (m_hp <= 0) {
        Defeat(m_orbDropWeight, m_dropPercent);
    }
}
