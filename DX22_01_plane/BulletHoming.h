#pragma once
#include <algorithm>
#include <cmath>

namespace BulletHoming
{
    // 誘導弾の調整値。ゲーム側とスキル説明画面の両方がこの値を参照する。
    // 通常弾の速度・寿命は変えず、Lv1～5で1秒あたり24～120度まで曲げる。
    constexpr int MaxRank = 5;
    constexpr float SearchRadius = 600.0f;

    // レベルを有効範囲に収めてから、1秒間に曲がれる最大角度を返す。
    // 未取得のLv0では0度となり、通常弾の直進を維持する。
    constexpr float DegreesPerSecond(int rank)
    {
        if (rank < 0)
        {
            rank = 0;
        }
        else if (rank > MaxRank)
        {
            rank = MaxRank;
        }

        return 24.0f * rank;
    }

    // 計算を描画から独立させる。ゲームではSimpleMath::Vector3を使用する。
    template <class Vector>
    class Search
    {
        // Searchは弾の更新ごとに作り直す。敵のポインタを保持しないため、
        // 前フレームで倒された敵を参照したり、特定の敵へ固定ロックしたりしない。
        Vector m_velocity;
        Vector m_offset{}; // 弾から、現在の最短候補へ向かうベクトル。
        float m_distanceSquared = SearchRadius * SearchRadius; // 最短候補までの距離の二乗。
        bool m_found = false;

    public:
        explicit Search(const Vector& velocity) : m_velocity(velocity)
        {
        }

        // 敵を1体ずつ候補として渡す。offset = 敵の中心座標 - 弾の座標。
        void Consider(const Vector& offset)
        {
            const float distanceSquared = offset.LengthSquared();

            // 通過済みの敵は追わず、その先にいる敵を新たな誘導先にする。
            // 内積が正なら弾の前方、0以下なら真横または後方にいる。
            // 距離の二乗で比較することで、候補ごとの平方根計算を省く。
            if (distanceSquared <= 0.000001f || distanceSquared > m_distanceSquared ||
                m_velocity.Dot(offset) <= 0.0f)
            {
                return;
            }

            // 今までより近い有効な候補が見つかったので、探索結果を更新する。
            m_found = true;
            m_distanceSquared = distanceSquared;
            m_offset = offset;
        }

        bool Found() const
        {
            return m_found;
        }

        const Vector& Offset() const
        {
            return m_offset;
        }
    };

    template <class Vector>
    Vector Turn(const Vector& velocity, const Vector& offset, int rank)
    {
        // 速度から「速さ」と「向き」を分離し、向きだけを変更する。
        const float speed = velocity.Length();
        const float distance = offset.Length();

        // 長さ0のベクトルは正規化できない。誘導不可の場合は元の速度を返す。
        if (rank <= 0 || speed < 0.000001f || distance < 0.000001f || velocity.Dot(offset) <= 0.0f)
        {
            return velocity;
        }

        const Vector forward = velocity / speed;
        const Vector target = offset / distance;

        // 単位ベクトル同士の内積は角度のcos値になる。
        // 浮動小数の丸めで1を超えるとacosが計算できないため、範囲を制限する。
        const float dot = std::clamp(forward.Dot(target), 0.0f, 1.0f);
        constexpr float Pi = 3.14159265358979323846f;

        // 既存の移動・寿命と同じ60更新/秒。一定角度以上は曲げず、必中にしない。
        // 度/秒 → ラジアン/秒 → ラジアン/フレームの順に単位を変換する。
        const float step = DegreesPerSecond(rank) * Pi / 180.0f / 60.0f;

        // すでに目標の向きが旋回上限内なら、その向きに合わせて行き過ぎを防ぐ。
        if (std::acos(dot) <= step)
        {
            return target * speed;
        }

        // 目標方向から前方成分を除くと「目標へ曲がる側」の直交方向が得られる。
        // この方向と現在の前方を使い、上限角度stepだけ回転した向きを作る。
        Vector tangent = target - forward * dot;
        if (tangent.LengthSquared() < 0.000001f)
        {
            return velocity;
        }
        tangent.Normalize();

        Vector result = forward * std::cos(step) + tangent * std::sin(step);
        result.Normalize(); // 誘導を繰り返しても弾速を変えない。
        return result * speed;
    }
}
