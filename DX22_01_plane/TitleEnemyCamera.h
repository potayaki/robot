/// @brief 展示台の通常カメラと、実際の敵フィギュアを左に収める観察カメラ。
#pragma once
#include <SimpleMath.h>
#include <algorithm>
#include <cmath>

class TitleEnemyCamera {
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Matrix = DirectX::SimpleMath::Matrix;

public:
    // 台座ローカル空間。中心と寸法は、その方向から見たモデルの投影範囲から求める。
    struct Target {
        Vector3 center{};
        Vector3 toEye{0, 0, -1};
        Vector2 size{1, 1};
    };

    static float HomeViewHeight(float aspect) {
        return (std::max)(19.0f, 23.5f / (std::max)(aspect, 0.1f));
    }

    void Focus(const Target& target, const Matrix& turntable) {
        const auto toEye = Vector3::TransformNormal(target.toEye, turntable);
        Pose next = {};
        next.center = Vector3::Transform(target.center, turntable);
        next.heading = std::atan2(toEye.x, toEye.z);
        next.elevation = std::asin(std::clamp(toEye.y, -1.0f, 1.0f));
        next.size = target.size;
        next.inspection = 1.0f;
        if (m_goal.inspection == 0 || m_goal.center != next.center || m_goal.heading != next.heading) {
            Start(next);
        }
    }

    void Leave() {
        if (m_goal.inspection > 0) {
            Start(Home());
        }
    }

    bool Active() const {
        return m_goal.inspection > 0 || m_view.inspection > 0;
    }

    float InspectionWeight() const {
        return m_view.inspection;
    }

    // 移動元がまだ画面中央にある間は残し、カメラがそこを離れるにつれて他の敵を隠す。
    float OtherFigureFade(const Vector3& center) const {
        const float width = (std::max)(m_view.size.x, m_view.size.y);
        const float distance = Vector3::Distance(m_view.center, center) / (std::max)(width, 0.1f);
        return m_view.inspection * std::clamp(distance, 0.0f, 1.0f);
    }

    Vector3 EyeDirection() const {
        const float horizontal = std::cos(m_view.elevation);
        return {horizontal * std::sin(m_view.heading), std::sin(m_view.elevation),
                horizontal * std::cos(m_view.heading)};
    }

    void Update(float dt) {
        if (m_time >= 1) {
            return;
        }
        m_time = (std::min)(1.0f, m_time + (std::max)(0.0f, dt) / 0.65f);
        const float t = m_time * m_time * (3 - 2 * m_time);
        m_view.center = Vector3::Lerp(m_from.center, m_goal.center, t);
        m_view.heading =
            m_from.heading + std::remainder(m_goal.heading - m_from.heading, DirectX::XM_2PI) * t;
        m_view.elevation = std::lerp(m_from.elevation, m_goal.elevation, t);
        m_view.size = Vector2::Lerp(m_from.size, m_goal.size, t);
        m_view.inspection = std::lerp(m_from.inspection, m_goal.inspection, t);
    }

    void Matrices(float aspect, Matrix& view, Matrix& projection) const {
        aspect = (std::max)(aspect, 0.1f);
        const float subjectHeight = (std::max)(m_view.size.y / 0.56f, m_view.size.x / (0.29f * aspect));
        const float height = std::lerp(HomeViewHeight(aspect), subjectHeight, m_view.inspection);
        const Vector3 toEye = EyeDirection();
        Vector3 right = Vector3::Up.Cross(-toEye);
        right.Normalize();
        // 視線を右へ平行移動し、敵の投影中心を画面幅の27%の位置に置く。
        const auto target = m_view.center + right * (0.23f * height * aspect * m_view.inspection);
        view = DirectX::XMMatrixLookAtLH(target + toEye * 45.25f, target, Vector3::Up);
        projection = DirectX::XMMatrixOrthographicLH(height * aspect, height, 0.1f, 150.0f);
    }

    // 手前で敵の輪郭に重なる装飾や別のフィギュアだけを観察中の描画から外す。
    bool Occludes(const Vector3& position, float radius) const {
        if (m_view.inspection < 0.55f) {
            return false;
        }
        const auto toEye = EyeDirection();
        const auto delta = position - m_view.center;
        const float depth = delta.Dot(toEye);
        if (depth < -radius) {
            return false;
        }
        const float padding = 0.55f * (std::max)(m_view.size.x, m_view.size.y);
        return (delta - toEye * depth).LengthSquared() < (radius + padding) * (radius + padding);
    }

private:
    struct Pose {
        Vector3 center{};
        float heading = 0;
        float elevation = 0;
        Vector2 size{1, 1};
        float inspection = 0;
    };

    static Pose Home() {
        Pose home;
        home.center = {0, 1.25f, 0};
        home.heading = std::atan2(24.0f, -34.0f);
        home.elevation = std::atan2(17.75f, std::hypot(24.0f, 34.0f));
        return home;
    }

    void Start(const Pose& next) {
        m_from = m_view;
        m_goal = next;
        // 通常カメラには被写体寸法がないので、出入りの間は観察対象の値をそのまま使う。
        if (m_from.inspection == 0) {
            m_from.size = next.size;
        }
        if (next.inspection == 0) {
            m_goal.size = m_from.size;
        }
        m_time = 0;
    }

    Pose m_view = Home(), m_from = m_view, m_goal = m_view;
    float m_time = 1;
};
