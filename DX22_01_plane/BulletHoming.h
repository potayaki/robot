/// @file BulletHoming.h
/// @brief 通常弾の誘導範囲と、速度を保ちながら曲げる処理。

#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace BulletHoming {
    // 誘導弾の調整値。ゲーム側とスキル説明画面の両方がこの値を参照する。
    // 通常弾の速度・寿命は変えず、Lv1～5で1秒あたりHoming～Homig×5度まで曲げる。
    constexpr int MaxRank = 5;
    constexpr float SearchRadius = 600.0f;
    constexpr float Homing = 10.0f;

    // Lv0は64px画像の中央にある4pxの白点だけを表示する。
    // Lv1以降はcrosshair061.pngを32, 48, 64, 80, 96の順に拡大する。
    constexpr float CrosshairSize(int rank) {
        rank = std::clamp(rank, 0, MaxRank);
        return rank == 0 ? 64.0f : 32.0f + 16.0f * (rank - 1);
    }

    constexpr float CursorRadius(int rank) {
        // crosshair061.pngの枠の内側は、64px画像の中心から約28px。
        // Lv0は点を表示するだけで、誘導範囲は持たない。
        return rank <= 0 ? 0.0f : CrosshairSize(rank) * (28.0f / 64.0f);
    }

    // レベルを有効範囲に収めてから、1秒間に曲がれる最大角度を返す。
    // 未取得のLv0では0度となり、通常弾の直進を維持する。
    constexpr float DegreesPerSecond(int rank) {
        if (rank < 0) {
            rank = 0;
        } else if (rank > MaxRank) {
            rank = MaxRank;
        }

        return Homing * rank;
    }

    // 計算を描画から独立させる。ゲームではSimpleMath::Vector3を使用する。
    template <class Vector> class Search {
        // Searchは弾の更新ごとに作り直す。敵のポインタを保持しないため、
        // 前フレームで倒された敵を参照したり、特定の敵へ固定ロックしたりしない。
        /// 位置を変化させる速度ベクトル（ワールド単位/秒）。
        Vector m_velocity;
        /// 弾から、照準の中心に最も近い候補へ向かうベクトル。
        Vector m_offset{};
        /// 現在の候補と照準中心の画面上の距離の二乗。
        float m_cursorDistanceSquared = std::numeric_limits<float>::infinity();
        /// 現在の候補までのワールド距離の二乗。
        float m_distanceSquared = SearchRadius * SearchRadius;
        /// 条件に合う敵が見つかったか。
        bool m_found = false;

    public:
        explicit Search(const Vector& velocity) : m_velocity(velocity) {}

        // 弾用カーソル内の敵だけを渡す。画面上の照準中心からの距離を優先する。
        // 同じ位置に重なっているときだけ、弾から近い方を選ぶ。
        void Consider(const Vector& offset, float cursorDistanceSquared) {
            const float distanceSquared = offset.LengthSquared();

            // 通過済みの敵は追わず、その先にいる敵を新たな誘導先にする。
            // 内積が正なら弾の前方、0以下なら真横または後方にいる。
            // 距離の二乗で比較することで、候補ごとの平方根計算を省く。
            if (distanceSquared <= 0.000001f || distanceSquared > SearchRadius * SearchRadius ||
                m_velocity.Dot(offset) <= 0.0f || !std::isfinite(cursorDistanceSquared) ||
                cursorDistanceSquared < 0.0f) {
                return;
            }

            if (cursorDistanceSquared > m_cursorDistanceSquared ||
                (cursorDistanceSquared == m_cursorDistanceSquared && distanceSquared >= m_distanceSquared)) {
                return;
            }

            // 弾から遠くても、照準の中心に近い敵なら新しい候補として採用する。
            m_found = true;
            m_cursorDistanceSquared = cursorDistanceSquared;
            m_distanceSquared = distanceSquared;
            m_offset = offset;
        }

        bool Found() const {
            return m_found;
        }

        const Vector& Offset() const {
            return m_offset;
        }
    };

    template <class Vector>
    Vector Turn(const Vector& velocity, const Vector& offset, int rank, float deltaSeconds) {
        // 速度から「速さ」と「向き」を分離し、向きだけを変更する。
        const float speed = velocity.Length();
        const float distance = offset.Length();

        // 長さ0のベクトルは正規化できない。誘導不可の場合は元の速度を返す。
        if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds) || rank <= 0 || speed < 0.000001f ||
            distance < 0.000001f || velocity.Dot(offset) <= 0.0f) {
            return velocity;
        }

        const Vector forward = velocity / speed;
        const Vector target = offset / distance;

        // 単位ベクトル同士の内積は角度のcos値になる。
        // 浮動小数の丸めで1を超えるとacosが計算できないため、範囲を制限する。
        const float dot = std::clamp(forward.Dot(target), 0.0f, 1.0f);
        constexpr float Pi = 3.14159265358979323846f;

        // 1秒あたりの旋回上限に実際の経過秒数を掛ける。
        // FPSによって誘導力が変わらず、停止中（0秒）は向きも変わらない。
        const float step = DegreesPerSecond(rank) * Pi / 180.0f * deltaSeconds;

        // すでに目標の向きが旋回上限内なら、その向きに合わせて行き過ぎを防ぐ。
        if (std::acos(dot) <= step) {
            return target * speed;
        }

        // 目標方向から前方成分を除くと「目標へ曲がる側」の直交方向が得られる。
        // この方向と現在の前方を使い、上限角度stepだけ回転した向きを作る。
        Vector tangent = target - forward * dot;
        if (tangent.LengthSquared() < 0.000001f) {
            return velocity;
        }
        tangent.Normalize();

        Vector result = forward * std::cos(step) + tangent * std::sin(step);
        result.Normalize(); // 誘導を繰り返しても弾速を変えない。
        return result * speed;
    }
} // namespace BulletHoming
