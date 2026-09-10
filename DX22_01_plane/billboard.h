#pragma once
#include "Object.h"
#include"Texture.h"
#include"Material.h"
#include"VertexBuffer.h"
#include"IndexBuffer.h"
#include<vector>
#include<string>
#include<memory>
#include<map>
class billboard : public Object
{
private:
    static std::vector<Texture*>m_sharedTextures;
    static std::map<std::string, std::vector<Texture*>> s_textureGroups; //
    std::string m_groupKey; // 自分がどのグループの画像を使うか
    static int m_instanceCount; 

    //板ポリゴンの描画データ
    std::vector<VERTEX_3D> m_Vertices;
    std::vector<unsigned int> m_Indices;
    VertexBuffer<VERTEX_3D> m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    std::unique_ptr<Material> m_Material;

    // --- アニメーション用データ ---
    int m_maxFrames;     // 総枚数
    int m_currentFrame;  // 現在表示している画像の番号
    float m_animTimer;   // 時間計測用タイマー
    float m_animSpeed;   // 何秒で次の画像に変えるか
    bool m_loop;         // ループ再生するかどうか

    int m_SplitX = 0;// 画像を分割するX方向の数
    int m_SplitY = 0;// 画像を分割するY方向の数

public:
    billboard();
    ~billboard();

    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;

    // アニメーション設定（速度とループ）
    void SetAnim(float animSpeed, bool loop = false);

    // 最初の一回だけ画像をまとめて読み込む静的関数
    static void LoadTextures(const std::string& groupKey,const std::string& baseName, const std::string& ext, int count);

    static void ReleaseTextures(const std::string& groupKey); // 静的に読み込んだ画像を解放する関数

    void Play(DirectX::SimpleMath::Vector3 pos) {
        m_Position = pos;
        m_currentFrame = -1;
        m_animTimer = 999.0f;
    }

    bool IsFinished()const {
        // ループ再生ではなく、かつ現在のコマが最大枚数に達していたら終了
        return (!m_loop && m_currentFrame >= m_maxFrames - 1);
    }

    void SetSpriteSheet(int splitX, int splitY,int maxFrame) {
        m_SplitX = splitX;
        m_SplitY = splitY;
        m_maxFrames = maxFrame;
    }

    void SetTextureGroup(const std::string& groupKey) { m_groupKey = groupKey; }

};

