/// @file PoolManager.h
/// @brief ゲームオブジェクトを再利用して生成負荷を抑えるプール。

#pragma once
#include "Object.h"
#include <vector>

template <typename T, int Max_Size> class PoolManager : public Object {
private:
    /// プールのリスト
    std::vector<T*> m_pool;
    /// 全要素が使用中の場合に次に再利用するプール内の番号。
    int m_currentIndex = 0;

public:
    /// プールの最大サイズを定数として公開
    static const int MAXSIZE = Max_Size;

    PoolManager() {}

    ~PoolManager() {
        Uninit();
    } // ← 修正: チルダは全角から半角へ

    /// 使用前の状態と必要な資源を初期化する。
    void Init() override {
        for (int i = 0; i < Max_Size; i++) {
            T* obj = new T();
            obj->Init();
            obj->SetActive(false); // 初期状態では非アクティブにする
            m_pool.push_back(obj);
        }
    }

    /// 経過時間と入力に合わせて状態を更新する。
    void Update() override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Update();
            }
        }
    }

    /// 現在の状態を画面へ描画する。
    void Draw(Camera* cam) override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Draw(cam);
            }
        }
    }

    /// 使用した資源を解放して終了する。
    void Uninit() override {
        for (auto& obj : m_pool) {
            if (obj != nullptr) {
                obj->Uninit();
                delete obj;
            }
        }
        m_pool.clear();
    }

    T* Spawn() {
        for (auto& obj : m_pool) {
            if (!obj->GetActive()) {
                obj->SetActive(true);
                return obj;
            }
        }

        T* obj = m_pool[m_currentIndex];
        obj->SetActive(true);
        m_currentIndex = (m_currentIndex + 1) % Max_Size; // 次のインデックスへ進める
        return obj;
    }

    //Debug
    int GetActiveCount() const {
        int count = 0;
        for (const auto& obj : m_pool) {
            if (obj->GetActive()) {
                count++;
            }
        }
        return count;
    }

    // プールの中身を外部から確認するための関数
    const std::vector<T*>& GetPool() const {
        return m_pool;
    }
};
