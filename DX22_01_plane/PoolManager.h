#pragma once
#include"Object.h"
#include<vector>

template<typename T, int Max_Size>
class PoolManager : public Object
{
private:
    std::vector<T*> m_pool; // プールのリスト

public:
    PoolManager() {}
    ~PoolManager() { Uninit(); } // ← 修正: チルダは全角から半角へ

    void Init() override {
        for (int i = 0; i < Max_Size; i++) {
            T* obj = new T();
            obj->Init();
            obj->SetActive(false); // 初期状態では非アクティブにする
            m_pool.push_back(obj);
        }
    }
    void Update() override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Update();
            }
        }
    }
    void Draw(Camera* cam) override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Draw(cam);
            }
        }
    }
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
        return nullptr; // 全て使用中の場合は nullptr
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

};      
