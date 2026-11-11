/// @file billboard.h
/// @brief カメラに向く板ポリゴンによる連番画像アニメーション。

#pragma once
/*
3D空間において、常にカメラの方を向く板ポリゴンを描画するクラス
爆発、煙、花火などのエフェクトに使用する。
*/
#include "Object.h"
#include "Texture.h"
#include "Material.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <vector>
#include <string>
#include <memory>
#include <map>

class billboard : public Object {
private:
    //--------------
    //静的テクスチャ管理
    //--------------
    /// 同じアニメーションを使うオブジェクト間で共有する画像一覧。
    static std::vector<Texture*> m_sharedTextures;
    /// フォルダや画像名をキーにした、アニメーション画像の共有キャッシュ。
    static std::map<std::string, std::vector<Texture*>> m_textureGroups;
    /// 自分がどのグループの画像を使うか
    std::string m_groupKey;
    /// 共有資源を使用しているインスタンス数。最後の終了時に資源を解放する。
    static int m_instanceCount;

    //--------------
    //板ポリゴンの描画データ
    //--------------
    /// 描画に使用する頂点の配列。
    std::vector<VERTEX_3D> m_vertices;
    /// 三角形を構成する頂点番号の配列。
    std::vector<unsigned int> m_indices;
    /// 描画頂点を保持するGPUバッファ。
    VertexBuffer<VERTEX_3D> m_vertexBuffer;
    /// 描画する頂点の順番を保持するGPUバッファ。
    IndexBuffer m_indexBuffer;
    /// 描画時に使用する色・反射・テクスチャ有効状態。
    std::unique_ptr<Material> m_material;

    //--------------
    // --- アニメーション用データ ---
    //--------------
    /// 総枚数
    int m_maxFrames;
    /// 現在表示している画像の番号
    int m_currentFrame;
    /// 時間計測用タイマー
    float m_animTimer;
    /// 何秒で次の画像に変えるか
    float m_animSpeed;
    /// 最終コマの表示時間まで終わったか。
    bool m_finished = false;
    void UpdateSpriteUV();
    /// ループ再生するかどうか
    bool m_loop;

    /// 画像を分割するX方向の数
    int m_splitX = 0;
    /// 画像を分割するY方向の数
    int m_splitY = 0;

public:
    billboard();
    ~billboard();

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override;
    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override;
    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override;
    /// 使用した資源を解放して終了する。
    void Uninit() override;

    // アニメーション設定（速度とループ）
    void SetAnim(float animSpeed, bool loop = false);

    // 最初の一回だけ画像をまとめて読み込む静的関数
    static void LoadTextures(const std::string& groupKey, const std::string& baseName, const std::string& ext,
                             int count);

    static void ReleaseTextures(const std::string& groupKey); // 静的に読み込んだ画像を解放する関数

    void Play(DirectX::SimpleMath::Vector3 pos) {
        m_position = pos;
        m_currentFrame = 0;
        m_animTimer = 0.0f;
        m_finished = false;
        UpdateSpriteUV(); // 再利用した瞬間から最初のコマを表示する。
    }

    bool IsFinished() const {
        // ループ再生ではなく、かつ現在のコマが最大枚数に達していたら終了
        return m_finished;
    }

    void SetSpriteSheet(int splitX, int splitY, int maxFrame) {
        m_splitX = splitX;
        m_splitY = splitY;
        m_maxFrames = maxFrame;
        UpdateSpriteUV();
    }

    void SetTextureGroup(const std::string& groupKey) {
        m_groupKey = groupKey;
    }
};
