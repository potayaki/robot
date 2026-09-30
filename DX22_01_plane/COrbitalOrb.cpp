#include "COrbitalOrb.h"
#include <DirectXMath.h>
#include"Game.h"
#include"CEnemy.h"

COrbitalOrb::COrbitalOrb() {
    m_body = nullptr;
    m_parentPlayer = nullptr;
    m_angle = 0.0f;
    m_radius = 5.0f;
    m_speed = 0.05f; // 毎フレームの回転スピード
}

COrbitalOrb::~COrbitalOrb() {
    Uninit();
}

void COrbitalOrb::Init() {
    m_body = new TestCube();
    m_body->Init();
    // プレイヤー(1.0f)より少し小さくする
    m_body->SetScale(0.5f, 0.5f, 0.5f);
}

void COrbitalOrb::SetOrbitalParam(Object* player, float startAngle, float radius, float speed) {
    m_parentPlayer = player;
    m_angle = startAngle;
    m_radius = radius;
    m_speed = speed;
}

void COrbitalOrb::Update() {
    // 基準となるプレイヤーがいなくなったら自身も消滅させる
    if (m_parentPlayer == nullptr || m_parentPlayer->IsDead()) {
        Destroy();
        return;
    }

    // 角度の更新（ラジアン）
    m_angle += m_speed;
    if (m_angle > DirectX::XM_2PI) {
        m_angle -= DirectX::XM_2PI;
    }

    // プレイヤーの現在位置を取得
    DirectX::SimpleMath::Vector3 playerPos = m_parentPlayer->GetPosition();

    // プレイヤーの周囲を回るXZ座標を計算
    m_Position.x = playerPos.x + m_radius * cosf(m_angle);
    m_Position.y = playerPos.y + 1.0f; // プレイヤーの少し上に浮かす
    m_Position.z = playerPos.z + m_radius * sinf(m_angle);

    // 見た目のモデルに座標を適用
    if (m_body) {
        m_body->SetPosition(m_Position.x, m_Position.y, m_Position.z);
    }

    //--------------------
    // 敵との当たり判定
    //--------------------
    // クールダウンを減らす
    if (m_cooldownTime > 0.0f) {
        m_cooldownTime -= 1.0f; // 1フレームごとに減らす
    }

    // クールダウンが0の時だけ攻撃判定を行う
    if (m_cooldownTime <= 0.0f) {
        // ゲーム上のすべての敵を取得
        std::vector<CEnemy*> enemies = Game::GetInstance()->GetObjects<CEnemy>();

        for (CEnemy* enemy : enemies) {
            // 敵が死んでいる、または非アクティブなら判定しない
            if (enemy == nullptr || enemy->IsDead() || !enemy->GetActive()) {
                continue;
            }

            // オーブと敵の距離を計算
            DirectX::SimpleMath::Vector3 diff = m_Position - enemy->GetPosition();
            float distanceSq = diff.LengthSquared();

            // お互いの半径（当たり判定のサイズ）を足す
            float myRadius = GetCollisionSphere().radius; // オーブの半径 (2.0f)
            float enemyRadius = enemy->GetCollisionSphere().radius;
            float hitRange = myRadius + enemyRadius;

            // 距離が当たり判定の範囲内ならヒット
            if (distanceSq <= (hitRange * hitRange)) {

                enemy->OnHit(m_damage);
                // 連続ヒットを防ぐため、クールダウンを設定（例：30フレーム = 0.5秒間は判定を消す）
                m_cooldownTime = 30.0f;

                
                // 貫通して複数に当たる仕様なら break は書かない
                //break;
            }
        }
    }
}

void COrbitalOrb::Draw(Camera* cam) {
    if (m_body) {
        m_body->Draw(cam);
    }
}

void COrbitalOrb::Uninit() {
    if (m_body) {
        m_body->Uninit();
        delete m_body;
        m_body = nullptr;
    }
}

Collision::Sphere COrbitalOrb::GetCollisionSphere() {
    // 独自の当たり判定サイズを返す（敵との接触判定用）
    return { m_Position,8.0f };
}

