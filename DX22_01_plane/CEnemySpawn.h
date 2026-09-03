#pragma once
#include "Object.h"
class CEnemySpawn :  public Object
{
private:
    int FrameCount = 0; // 現在のフレーム数
public:

    int m_SpawnInterval = 120; // 敵を出現させる間隔（フレーム数）

    float m_RadiusMin = 500.0f; // 出現する円の最小半径
    float m_RadiusMax = 1000.0f; // 出現する円の最大半径

    int m_EnemyCount = 5; // 出現させる敵の数

    
    int m_MaxEnemyCount = 30;//敵の最大表示数

    CEnemySpawn();
    ~CEnemySpawn();
    void Init() override;
    void Update() override;
    void Draw(Camera* cam) override;
    void Uninit() override;
};

