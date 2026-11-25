/// @file ExplosionManager.h
/// @brief 再利用可能な爆発エフェクトの管理。

#pragma once
#include "Object.h"
#include "billboard.h"

#ifdef _DEBUG
#include <deque>
#include <vector>

struct MissileBlastDebugHit {
    /// 着弾時の敵の当たり判定中心（ワールド座標）。
    DirectX::SimpleMath::Vector3 center;
    /// 敵に渡したダメージ。敵自身の防御補正前の値。
    int damage;
    /// 当たり判定が全ダメージの内球と重なったか。
    bool full;
};

struct MissileBlastDebugSample {
    /// ステージ開始からの着弾番号と、着弾したゲーム内時刻（秒）。
    unsigned id = 0;
    double gameTime = 0;
    /// 爆心（ワールド座標）と、発射時の強化を反映した内外の半径。
    DirectX::SimpleMath::Vector3 center;
    float fullRadius = 0;
    float outerRadius = 0;
    /// 内球と外周それぞれで敵に渡すダメージ。
    int fullDamage = 0;
    int outerDamage = 0;
    /// 実際に命中した内球と外周の敵数。
    int fullHits = 0;
    int outerHits = 0;
    std::vector<MissileBlastDebugHit> hits;
};
#endif

class ExplosionManager : public Object {
private:
    /// 画面に出せる最大爆発数
    static const int POOL_SIZE = 20;
    /// 再利用するオブジェクトを保持する一覧。
    billboard* m_pool[POOL_SIZE];
    /// 更新・描画の対象として有効になっているか。
    bool m_active[POOL_SIZE];

#ifdef _DEBUG
    /// 新しい順に着弾番号を発行し、直近の一斉射を振り返れる件数だけ保存する。
    static constexpr size_t MaxDebugMissileBlasts = 20;
    unsigned m_nextDebugBlastId = 1;
    std::deque<MissileBlastDebugSample> m_debugMissileBlasts;
#endif

public:
    /// 未強化の爆発画像の大きさ（ワールド単位）。
    static constexpr float BaseDisplaySize = 200.0f;

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    /// 指定位置で爆発を再生する。大きさの倍率は再利用するたびに設定する。
    void CreateExplosion(DirectX::SimpleMath::Vector3 pos, float sizeMultiplier = 1.0f);

#ifdef _DEBUG
    /// 選択した着弾の内外球と命中位置をワールドに描くか。
    bool m_debugShowMissileBlast = false;
    /// 0なら最新に追従。正の番号なら保存済みの対応する着弾を表示する。
    unsigned m_debugSelectedBlastId = 0;

    void RecordDebugMissileBlast(MissileBlastDebugSample sample);

    const std::deque<MissileBlastDebugSample>& GetDebugMissileBlasts() const {
        return m_debugMissileBlasts;
    }

    const MissileBlastDebugSample* GetDisplayedDebugMissileBlast() const;
    void ClearDebugMissileBlasts();
#endif
};
