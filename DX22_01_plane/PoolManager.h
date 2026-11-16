/// @file PoolManager.h
/// @brief ゲームオブジェクトを再利用して生成負荷を抑えるプール。

#pragma once
#include "Object.h"
#include <vector>

template <typename T, int Capacity> class PoolManager : public Object {
private:
    /// プールが所有する全要素。非アクティブな要素も、次の発射やエフェクト生成まで保持する。
    std::vector<T*> m_pool;
    /// 空きがないときに次に使い直す要素の番号。末尾まで使ったら先頭に戻る。
    int m_nextReuseIndex = 0;

public:
    /// 初期化時に用意する個数。デバッグ表示でも同じ値を上限に使う。
    static constexpr int MAXSIZE = Capacity;

    ~PoolManager() override {
        Uninit();
    }

    /// 各要素の Init を一度ずつ呼び、すべてを待機状態にする。
    void Init() override {
        for (int i = 0; i < Capacity; i++) {
            T* obj = new T();
            obj->Init();
            obj->SetActive(false);
            m_pool.push_back(obj);
        }
    }

    /// 使用中の要素だけを更新する。
    void Update() override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Update();
            }
        }
    }

    /// 使用中の要素だけを指定カメラで描画する。
    void Draw(Camera* cam) override {
        for (auto& obj : m_pool) {
            if (obj->GetActive()) {
                obj->Draw(cam);
            }
        }
    }

    /// 各要素が持つ資源を手放してから、プール内の全要素を破棄する。
    void Uninit() override {
        for (auto& obj : m_pool) {
            if (obj != nullptr) {
                obj->Uninit();
                delete obj;
            }
        }
        m_pool.clear();
    }

    /// 最初の空きを使用中にする。満杯なら使用中の要素を順番に返すので、呼び出し元が初期状態を設定する。
    T* Spawn() {
        for (auto& obj : m_pool) {
            if (!obj->GetActive()) {
                obj->SetActive(true);
                return obj;
            }
        }

        T* obj = m_pool[m_nextReuseIndex];
        obj->SetActive(true);
        m_nextReuseIndex = (m_nextReuseIndex + 1) % Capacity;
        return obj;
    }

    /// 現在使用中の個数を数える。デバッグ表示から呼ばれる。
    int GetActiveCount() const {
        int count = 0;
        for (const auto& obj : m_pool) {
            if (obj->GetActive()) {
                count++;
            }
        }
        return count;
    }

    /// 当たり判定のデバッグ描画などで全要素を参照する。所有権はプールにある。
    const std::vector<T*>& GetPool() const {
        return m_pool;
    }
};
