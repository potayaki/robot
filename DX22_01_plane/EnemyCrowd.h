/// @file EnemyCrowd.h
/// @brief スライムが互いを通り抜けず、接触攻撃できる距離まで群れで接近する移動。

#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace EnemyCrowd {
    struct Neighbor {
        /// 隣の敵の判定球中心（ワールド座標）。分離にはXZだけを使う。
        DirectX::SimpleMath::Vector3 center;
        /// 隣の敵の判定半径（ワールド単位）。
        float radius;
        /// 同じ中心にいる個体を反対方向へ分けるための、個体ごとに異なる識別値。
        std::uintptr_t id;
    };

    /// 球が接触する距離よりわずかに近づけ、厳密な小なりの接触攻撃判定を維持する。
    inline constexpr float ContactRatio = 0.95f;
    /// 隣接する敵同士に残す余白（ワールド単位）。
    inline constexpr float BodyGap = 2.0f;
    /// 接近より分離を優先する強さ。半径が接する付近では反発を弱くする。
    inline constexpr float SeparationStrength = 60.0f;

    inline DirectX::SimpleMath::Vector3 Move(const DirectX::SimpleMath::Vector3& position,
                                             const DirectX::SimpleMath::Vector3& player, float radius,
                                             float playerRadius, float speed, float seconds,
                                             std::span<const Neighbor> neighbors, std::uintptr_t id) {
        using DirectX::SimpleMath::Vector3;
        if (!std::isfinite(seconds) || seconds <= 0 || !std::isfinite(speed) || speed <= 0) {
            return position;
        }
        const float limit = speed * seconds;
        if (!std::isfinite(limit)) {
            return position;
        }
        radius = (std::max)(0.0f, radius);
        const float contact = (radius + (std::max)(0.0f, playerRadius)) * ContactRatio;
        Vector3 toward = player - position;
        toward.y = 0;
        const float distance = toward.Length();
        Vector3 steering = Vector3::Zero;
        if (distance > contact && distance > 0.0001f) {
            steering = toward * ((std::min)(1.0f, (distance - contact) / limit) / distance);
        }

        // 分離による押し戻しを含めても、1更新の移動量は本来の速度×秒を超えない。
        const auto limitTravel = [&](Vector3 requested) {
            Vector3 movement = requested - position;
            movement.y = 0;
            const float length = movement.Length();
            if (length > limit) {
                movement *= limit / length;
            }
            return position + movement;
        };
        const auto separate = [&](const Vector3& center, float spacing, std::uintptr_t otherId) {
            Vector3 offset = position - center;
            offset.y = 0;
            const float lengthSquared = offset.LengthSquared();
            if (lengthSquared >= spacing * spacing) {
                return;
            }
            const float length = std::sqrt(lengthSquared);
            if (length > 0.001f) {
                offset /= length;
            } else {
                // 完全に重なった場合も正規化でNaNにせず、組ごとに逆向きへ押し分ける。
                const auto low = (std::min)(id, otherId);
                const auto high = (std::max)(id, otherId);
                const float angle =
                    static_cast<float>((low * 31u + high * 17u) % 1024u) * (DirectX::XM_2PI / 1024.0f);
                offset = Vector3(std::cos(angle), 0, std::sin(angle));
                if (id > otherId) {
                    offset = -offset;
                }
            }
            steering += offset * ((spacing - length) / spacing * SeparationStrength);
        };

        // 近隣からの反発を先に合計し、最後に一度だけ移動する。
        // 個別の押し戻しを重ねる方式では、後の隣接個体が前の分離を打ち消して重なってしまう。
        for (const auto& neighbor : neighbors) {
            if (neighbor.id != id) {
                separate(neighbor.center, radius + (std::max)(0.0f, neighbor.radius) + BodyGap, neighbor.id);
            }
        }
        if (distance < contact) {
            separate(player, contact, 0);
        }
        Vector3 result = limitTravel(position + steering * limit);
        // 周囲から押されてもプレイヤーの中心までめり込まない。
        Vector3 fromPlayer = result - player;
        fromPlayer.y = 0;
        const float finalDistance = fromPlayer.Length();
        if (finalDistance < contact && distance >= contact - 0.001f) {
            // プレイヤー円との最初の交点で移動を止める。端点の押し戻しでは
            // 移動量制限との組み合わせで再び円の中へ戻るため、移動線分を切る。
            const Vector3 movement = result - position;
            const Vector3 from = -toward;
            const float a = movement.LengthSquared();
            const float b = from.Dot(movement);
            const float c = (std::max)(0.0f, distance * distance - contact * contact);
            const float discriminant = b * b - a * c;
            if (a > 0.000001f && discriminant >= 0) {
                const float fraction = std::clamp((-b - std::sqrt(discriminant)) / a, 0.0f, 1.0f);
                result = position + movement * fraction;
            }
        }
        return result;
    }
} // namespace EnemyCrowd
