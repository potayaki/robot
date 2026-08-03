#pragma once
#include "Object.h"
#include"Texture.h"
#include"Material.h"
#include"VertexBuffer.h"
#include"IndexBuffer.h"
#include<vector>
#include<string>
#include<memory>
class billboard : public Object
{
private:
    static std::vector<Texture*>m_sharedTextures;
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
    static void LoadTextures(const std::string& baseName, const std::string& ext, int count);

};

