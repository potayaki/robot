/// @file Camera.cpp
/// @brief プレイヤー追従視点、カーソルのレイ、画面揺れ、自由カメラ。

#include "Renderer.h"
#include "Camera.h"
#include "Application.h"
#include "input.h"
#include "CPlayer.h"
#include "Game.h"
#include "AimCursor.h"
#include "ForestTerrain.h"
#include <algorithm>
#include <cmath>
#ifdef _DEBUG
#include "imgui.h"
#endif
using namespace DirectX::SimpleMath;

//=======================================
//初期化処理
//=======================================
void Camera::Init() {
    m_position = Vector3(0.0f, 20.0f, -50.0f);
    m_target = Vector3(0.0f, 0.0f, 0.0f);
    m_cameraDirection = 3.14f;
    m_shotKickPending = false;
    m_shotRecoilTime = 0.0f;

    m_width = Application::GetWidth();
}

//=======================================
//更新処理
//=======================================
void Camera::Update() {
    if (m_shotKickPending) {
        // バーストや連射でも1発分を上限にし、新しい反動を照準計算の前に確定する。
        m_shotRecoilTime = ShotRecoilSeconds;
        m_shotKickPending = false;
    } else {
        GameTime::CountDown(m_shotRecoilTime, GameTime::DeltaSeconds());
    }
    //Debug用のカメラ操作

#ifdef _DEBUG

    // 通常更新へ戻るときは、自由視点の座標をゲームの追従カメラへ持ち越さない。
    SetDebugFreeCamera(false);

    //デバック用のカメラ操作
    if (Input::GetKeyPress(VK_K)) {
        m_cameraDirection += m_rotationSpeed * GameTime::DeltaSeconds();
    }
    if (Input::GetKeyPress(VK_L)) {
        m_cameraDirection -= m_rotationSpeed * GameTime::DeltaSeconds();
    }

#endif // _DEBUG

    // 現在のウィンドウサイズ取得（TODO: Initで保存した値を使うべき）
    RECT clientRect;
    GetClientRect(Application::GetWindow(), &clientRect);
    int Width = clientRect.right - clientRect.left;

    //マウスの座標を取得(ウィンドウ座標に変換)
    POINT MousePos;
    GetCursorPos(&MousePos);
    ScreenToClient(Application::GetWindow(), &MousePos);

    //TODO : 画面端のどのくらいの幅で回転するか
    // 左右の画面端に入ったら、設定した速さで旋回する。
    const float RotationSpeed = m_rotationSpeed * GameTime::DeltaSeconds(); // ラジアン/秒から今回の回転量へ。

    //カメラの右回転
    if (MousePos.x >= Width - m_edgeWidth) {
        m_cameraDirection += RotationSpeed;
    } //カメラの左回転
    else if (MousePos.x <= m_edgeWidth) {
        m_cameraDirection -= RotationSpeed;
    }

    //カメラ位置を注視点＋回転オフセット
    Vector3 pos = m_target;
    pos.y += m_previewHeight;
    pos.x += sin(m_cameraDirection) * m_previewRadius;
    pos.z += cos(m_cameraDirection) * m_previewRadius;
    m_position = pos;

    // -------------------------------------
    //ステージ１ではプレイヤー追従カメラにする
    //-------------------------------------
    Scene* CurrentScene = Game::GetInstance()->GetScene();
    if (dynamic_cast<Stage1Scene*>(CurrentScene) != nullptr) {
        std::vector<CPlayer*> player = Game::GetInstance()->GetObjects<CPlayer>();
        if (!player.empty()) { //プレイヤーがいるとき
            Vector3 playerpos = player[0]->GetPosition();
            SetTarget(playerpos);

            //offsetのx,zを回転させる
            float radius = sqrt(m_followOffset.x * m_followOffset.x +
                                m_followOffset.z * m_followOffset.z); //初期値なら150
            Vector3 rotatedOffset;
            rotatedOffset.x = sin(m_cameraDirection) * radius;
            rotatedOffset.y = m_followOffset.y;
            rotatedOffset.z = cos(m_cameraDirection) * radius;

            m_position = playerpos + rotatedOffset; //プレイヤーの周囲を回転しながら追従
            // 下り坂でカメラが登り側へ回ったときも、足元への視線を地中に通さない。
            constexpr int samples = 6;
            constexpr float clearance = 7.0f;
            for (int i = 1; i <= samples; ++i) {
                const float t = static_cast<float>(i) / samples;
                const Vector3 point = playerpos + rotatedOffset * t;
                const float minY = ForestLayout::GroundHeight(point.x, point.z) + clearance;
                m_position.y = (std::max)(m_position.y, playerpos.y + (minY - playerpos.y) / t);
            }
        }
    }

    //-------------------------------------
    //カメラの揺れ
    //-------------------------------------
    if (m_shakeTime > 0.0f) {
        // -1.0 ～ 1.0 のランダムな乱数を作って、強さを掛け算する
        const float strength = m_shakePower * m_shakeTime / m_shakeDuration;
        float rx = ((rand() % 100) / 50.0f - 1.0f) * strength;
        float ry = ((rand() % 100) / 50.0f - 1.0f) * strength;
        float rz = ((rand() % 100) / 50.0f - 1.0f) * strength;

        // カメラ位置と注視点を同時に揺らすと画面全体が揺れる
        m_position.x += rx;
        m_position.y += ry;
        m_position.z += rz;

        m_target.x += rx;
        m_target.y += ry;
        m_target.z += rz;

        // タイマーを減らす
        GameTime::CountDown(m_shakeTime, GameTime::DeltaSeconds());
    }
}

#ifdef _DEBUG
void Camera::SetDebugFreeCamera(bool enabled) {
    if (m_debugFreeCamera == enabled) {
        return;
    }
    m_debugFreeCamera = enabled;

    if (enabled) {
        m_debugSavedPosition = m_position;
        m_debugSavedTarget = m_target;
        ResetDebugFreeCamera();
    } else {
        m_position = m_debugSavedPosition;
        m_target = m_debugSavedTarget;
    }
}

void Camera::ResetDebugFreeCamera() {
    m_position = m_debugSavedPosition;
    Vector3 forward = m_debugSavedTarget - m_debugSavedPosition;
    if (forward.LengthSquared() < 0.0001f) {
        forward = Vector3(0, 0, 1);
    }
    forward.Normalize();

    m_debugYawDegrees = DirectX::XMConvertToDegrees(std::atan2(forward.x, forward.z));
    m_debugPitchDegrees = DirectX::XMConvertToDegrees(std::asin(std::clamp(forward.y, -1.0f, 1.0f)));
    ApplyDebugFreeLook();
}

void Camera::ApplyDebugFreeLook() {
    // 真上・真下では注視方向と上方向が重なるため、少し手前で止める。
    m_debugPitchDegrees = std::clamp(m_debugPitchDegrees, -89.0f, 89.0f);
    m_debugYawDegrees = std::remainder(m_debugYawDegrees, 360.0f);
    const float yaw = DirectX::XMConvertToRadians(m_debugYawDegrees);
    const float pitch = DirectX::XMConvertToRadians(m_debugPitchDegrees);
    m_target = m_position +
               Vector3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
}

void Camera::UpdateDebugFreeCamera() {
    if (!m_debugFreeCamera || GetForegroundWindow() != Application::GetWindow()) {
        return;
    }

    // スキル選択や装備詳細のダイアログを操作している間は、視点入力を受け付けない。
    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
        return;
    }
    const ImGuiIO& io = ImGui::GetIO();

    // UI外の右ドラッグだけを視点操作に使う。数値欄やスライダーのドラッグとは分離する。
    if (!io.WantCaptureMouse && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        constexpr float degreesPerPixel = 0.2f;
        m_debugYawDegrees += io.MouseDelta.x * degreesPerPixel;
        m_debugPitchDegrees -= io.MouseDelta.y * degreesPerPixel;
        ApplyDebugFreeLook();
    }

    if (io.WantCaptureKeyboard || io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    Vector3 forward = m_target - m_position;
    forward.Normalize();
    Vector3 right = Vector3(0, 1, 0).Cross(forward);
    right.Normalize();

    Vector3 movement = Vector3::Zero;
    if (Input::GetKeyPress(VK_W)) {
        movement += forward;
    }
    if (Input::GetKeyPress(VK_S)) {
        movement -= forward;
    }
    if (Input::GetKeyPress(VK_D)) {
        movement += right;
    }
    if (Input::GetKeyPress(VK_A)) {
        movement -= right;
    }
    if (Input::GetKeyPress(VK_E)) {
        movement.y += 1.0f;
    }
    if (Input::GetKeyPress(VK_Q)) {
        movement.y -= 1.0f;
    }

    if (movement.LengthSquared() > 0.0001f) {
        // 斜め移動でも同じ速さにする。停止中はDeltaSecondsが0なので実時間を使う。
        movement.Normalize();
        const float speed = m_debugMoveSpeed * (Input::GetKeyPress(VK_SHIFT) ? 3.0f : 1.0f);
        m_position += movement * speed * GameTime::UnscaledDeltaSeconds();
        ApplyDebugFreeLook();
    }
}

void Camera::DrawDebugFreeCameraUI() {
    if (!m_debugFreeCamera) {
        return;
    }
    const auto text = [](const char8_t* value) { return reinterpret_cast<const char*>(value); };

    ImGui::Separator();
    ImGui::TextUnformatted(text(u8"一時停止中：フリーカメラ"));
    ImGui::TextWrapped("%s", text(u8"WASD：移動 / Q・E：下降・上昇 / Shift：高速移動"));
    ImGui::TextWrapped("%s", text(u8"画面上のUIがない場所で右ドラッグ：視点変更。数値でも調整できます。"));

    float position[3] = {m_position.x, m_position.y, m_position.z};
    bool changed = ImGui::DragFloat3(text(u8"カメラ位置"), position, 0.5f, 0, 0, "%.1f");
    if (changed) {
        m_position = Vector3(position[0], position[1], position[2]);
    }

    changed |= ImGui::DragFloat(text(u8"左右の角度"), &m_debugYawDegrees, 0.5f, -180, 180, "%.1f deg",
                                ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::DragFloat(text(u8"上下の角度"), &m_debugPitchDegrees, 0.5f, -89, 89, "%.1f deg",
                                ImGuiSliderFlags_AlwaysClamp);
    if (changed) {
        ApplyDebugFreeLook();
    }

    ImGui::SliderFloat(text(u8"カメラ移動速度"), &m_debugMoveSpeed, 5, 500, "%.0f / sec",
                       ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::Button(text(u8"一時停止した時の視点に戻す"))) {
        ResetDebugFreeCamera();
    }
    ImGui::TextDisabled("%s", text(u8"一時停止を解除すると通常の追従カメラに戻ります。"));
    ImGui::Separator();
}
#endif

//=======================================
//描画処理
//=======================================
void Camera::SetCamera(int mode) {
    if (mode == 0) { //３Dカメラ

        // ビュー変換後列作成
        Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
        const auto pose = GetViewPose();
        m_viewMatrix = DirectX::XMMatrixLookAtLH(pose.eye, pose.target, up); //左手系

        Renderer::SetViewMatrix(&m_viewMatrix);

        //プロジェクション行列の生成
        constexpr float fieldOfView = AimCursor::FieldOfView; // 照準判定と共通の視野角。

        float aspectRatio = static_cast<float>(Application::GetWidth()) /
                            static_cast<float>(Application::GetHeight()); // アスペクト比
        const float nearPlane = m_nearPlane; // ニアクリップ
        const float farPlane = m_farPlane; // ファークリップ

        //プロジェクション行列の生成
        Matrix projectionMatrix;
        projectionMatrix =
            DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane); //左手系

        Renderer::SetProjectionMatrix(&projectionMatrix);
    } else if (mode == 1) { //2D
        //ビュー変換行列作成
        Vector3 pos = Vector3(0.0f, 0.0f, -10.0f);
        Vector3 tgt = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
        m_viewMatrix = DirectX::XMMatrixLookAtLH(pos, tgt, up);
        Renderer::SetViewMatrix(&m_viewMatrix);

        //プロジェクション行列の作成
        float nearPlane = 1.0f;
        const float farPlane = m_uiFarPlane;
        Matrix projectionMatrix = DirectX::XMMatrixOrthographicLH(
            static_cast<float>(Application::GetWidth()), static_cast<float>(Application::GetHeight()),
            nearPlane, farPlane);
        projectionMatrix = DirectX::XMMatrixTranspose(projectionMatrix);
        Renderer::SetProjectionMatrix(&projectionMatrix);
    }
}

//=======================================
//終了処理
//=======================================
void Camera::Uninit() {}

void Camera::SetTarget(DirectX::SimpleMath::Vector3 target) {
    m_target = target;
}

Camera::ViewPose Camera::GetViewPose() const {
    ViewPose pose{m_position, m_target};
    if (m_shotRecoilTime <= 0.0f) {
        return pose;
    }
#ifdef _DEBUG
    if (m_debugFreeCamera) {
        return pose;
    }
#endif
    Vector3 forward = m_target - m_position;
    const float distance = forward.Length();
    if (distance < 0.0001f) {
        return pose;
    }
    forward /= distance;
    Vector3 screenUp = Vector3::UnitY - forward * forward.y;
    if (screenUp.LengthSquared() < 0.0001f) {
        return pose;
    }
    screenUp.Normalize();

    constexpr float pullDistance = 4.0f; // 発射直後の後退量（ワールド単位）。
    constexpr float pitch = DirectX::XMConvertToRadians(0.7f);
    const float remaining = m_shotRecoilTime / ShotRecoilSeconds;
    const float strength = remaining * remaining;
    const float pull = pullDistance * strength;
    pose.eye -= forward * pull;
    pose.target += screenUp * ((distance + pull) * std::tan(pitch) * strength);
    return pose;
}

//-------------------------------------
//マウスレイを取得
//-------------------------------------
AimCursor::Region Camera::GetAimCursor() const {
    const auto mouse = Input::GetMousePosition();
    const auto pose = GetViewPose();
    return AimCursor::Region(pose.eye, pose.target, Vector2(mouse.x, mouse.y),
                             static_cast<float>(Application::GetHeight()));
}

bool Camera::GetMouseRay(DirectX::SimpleMath::Vector3& rayOrigin,
                         DirectX::SimpleMath::Vector3& rayDirection) const {
    const auto mouse = Input::GetMousePosition();

    const float halfWidth = Application::GetWidth() * 0.5f; //ウィンドウ幅の半分
    const float halfHeight = Application::GetHeight() * 0.5f; //ウィンドウ高さの半分

    //-1.0 ～ 1.0 に正規化
    const float ndcX = mouse.x / halfWidth;
    // Y軸はウィンドウ座標と3D空間で上下逆転するため、マイナスをつける
    const float ndcY = mouse.y / halfHeight;

    // 3D空間での視野角とアスペクト比を考慮して、レイの方向を計算する
    constexpr float fov = AimCursor::FieldOfView;
    const float aspect =
        static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::GetHeight());

    const float tanHalfFov = tanf(fov * 0.5f);

    const auto pose = GetViewPose();
    Vector3 forward = pose.target - pose.eye;
    if (forward.LengthSquared() < 0.0001f) {
        return false;
    }
    forward.Normalize();

    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 right = up.Cross(forward);
    right.Normalize();

    up = forward.Cross(right);
    up.Normalize();

    // 画面中央 (ndcX=0, ndcY=0) なら真正面 (forward) に飛ぶようになる
    rayDirection = forward + right * (ndcX * aspect * tanHalfFov) + up * (ndcY * tanHalfFov);

    rayDirection.Normalize();
    rayOrigin = pose.eye;

    return true;
}
