/// @file Renderer.h
/// @brief Direct3Dの初期化、画面描画、シェーダー・GPU資源の共通処理。

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <d3d11.h>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <io.h>
#include <string>
#include <vector>
#include <d3dcompiler.h>
#include <locale.h>
#include "Renderer.h"

//外部ライブラリ
#pragma comment(lib, "directxtk.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Direct3D解放の簡略化マクロ
#define SAFE_RELEASE(p)                                                                                      \
    {                                                                                                        \
        if (NULL != p) {                                                                                     \
            p->Release();                                                                                    \
            p = NULL;                                                                                        \
        }                                                                                                    \
    }

// ３Ｄ頂点データ
struct VERTEX_3D {
    /// モデルまたはオブジェクトの位置。
    DirectX::SimpleMath::Vector3 position;
    /// 頂点または面の法線方向。光の当たり方の計算に使用する。
    DirectX::SimpleMath::Vector3 normal;
    /// 描画色。RGBAの各成分を保持する。
    DirectX::SimpleMath::Color color;
    /// テクスチャを参照するUV座標。
    DirectX::SimpleMath::Vector2 uv;
};

// ブレンドステート
enum EBlendState {
    BS_NONE = 0, // 半透明合成無し
    BS_ALPHABLEND, // 半透明合成
    BS_ADDITIVE, // 加算合成
    BS_SUBTRACTION, // 減算合成
    MAX_BLENDSTATE
};

struct LIGHT {
    /// 光を使うか否か
    BOOL Enable;
    /// HLSL定数バッファの16バイト境界に合わせるための余白。
    BOOL Dummy[3];
    /// 並行光源の方向
    DirectX::SimpleMath::Vector4 Direction;
    /// 並行光源の強さと色
    DirectX::SimpleMath::Color Diffuse;
    /// 環境光の強さと色
    DirectX::SimpleMath::Color Ambient;
};

//サブセット
struct SUBSET {
    /// この描画範囲に割り当てられた材質の名前。
    std::string MtrlName;
    /// この描画範囲が使用するインデックス数。
    unsigned int IndexNum = 0;
    /// この描画範囲が使用する頂点数。
    unsigned int VertexNum = 0;
    /// インデックス配列内でこの描画範囲が始まる位置。
    unsigned int IndexBase = 0;
    /// 頂点配列内でこの描画範囲が始まる位置。
    unsigned int VertexBase = 0;
    /// この描画範囲で使用する材質の番号。
    unsigned int MaterialIdx = 0;
};

//マテリアル
struct MATERIAL {
    /// 周囲の環境光に対する色と強さ。
    DirectX::SimpleMath::Color Ambient;
    /// 光源からの拡散反射の色と強さ。
    DirectX::SimpleMath::Color Diffuse;
    /// 鏡面反射の色と強さ。既存の材質構造に合わせた項目名。
    DirectX::SimpleMath::Color Speclar;
    /// 材質自身が発光する色と強さ。
    DirectX::SimpleMath::Color Emmision;
    /// 鏡面反射の鋭さ。
    float Shiness;
    /// テクスチャを使って描画するか。
    BOOL TextureEnable;
    /// HLSL定数バッファの16バイト境界に合わせるための余白。
    BOOL Dummy[2];
};

//-----------------------------------------------------------------------------
//Rendererクラス
//-----------------------------------------------------------------------------
class Renderer {
private:
    /// 作成したDirect3Dデバイスが対応する機能レベル。
    static D3D_FEATURE_LEVEL m_featureLevel;

    /// GPU資源の作成を担当するDirect3Dデバイス。
    static ID3D11Device* m_pDevice;
    /// 描画設定・データ転送・描画命令を発行するコンテキスト。
    static ID3D11DeviceContext* m_pDeviceContext;
    /// 描画した画面をウィンドウへ表示するスワップチェーン。
    static IDXGISwapChain* m_pSwapChain;
    /// 現在の画面描画先を表すビュー。
    static ID3D11RenderTargetView* m_pRenderTargetView;
    /// 画面の深度・ステンシル情報を参照するビュー。
    static ID3D11DepthStencilView* m_pDepthStencilView;

    /// モデルのワールド行列をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_pWorldBuffer;
    /// カメラのビュー行列をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_pViewBuffer;
    /// 透視投影行列をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_pProjectionBuffer;

    /// 光源の方向・色・環境光をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_pLightBuffer;
    /// 色や材質の情報をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_materialBuffer;
    /// テクスチャのUV範囲をGPUへ渡す定数バッファ。
    static ID3D11Buffer* m_pTextureBuffer;

    /// 通常の深度判定を有効にする設定。
    static ID3D11DepthStencilState* m_pDepthStateEnable;
    /// UIなどの描画で深度判定を無効にする設定。
    static ID3D11DepthStencilState* m_pDepthStateDisable;

    /// ブレンド ステート;
    static ID3D11BlendState* m_pBlendState[MAX_BLENDSTATE];
    /// アルファ値をマルチサンプルの被覆率へ変換する描画設定。
    static ID3D11BlendState* m_pBlendStateATC;

    static HRESULT CreateRenderAndDepthResources();

public:
    /// 使用前の状態と必要な資源を初期化する。
    static HRESULT Init();
    /// 使用した資源を解放して終了する。
    static void Uninit();
    static void DrawStart();
    static void DrawEnd();

    static HRESULT ResizeWindow(int width, int height);

    static void SetDepthEnable(bool Enable);

    static void SetATCEnable(bool Enable);

    static void SetWorldViewProjection2D();
    static void SetWorldMatrix(DirectX::SimpleMath::Matrix* WorldMatrix);
    static void SetViewMatrix(DirectX::SimpleMath::Matrix* ViewMatrix);
    static void SetProjectionMatrix(DirectX::SimpleMath::Matrix* ProjectionMatrix);

    static ID3D11Device* GetDevice(void) {
        return m_pDevice;
    }

    static ID3D11DeviceContext* GetDeviceContext(void) {
        return m_pDeviceContext;
    }

    static HRESULT CompileShader(const char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
                                 void** ppShaderObject, int* pShaderObjectSize);
    static HRESULT CreateVertexShader(ID3D11VertexShader** ppVertexShader, ID3D11InputLayout** ppVertexLayout,
                                      D3D11_INPUT_ELEMENT_DESC* pLayout, unsigned int numElements,
                                      const char* szFileName);
    static HRESULT CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

    static bool CreateIndexBuffer(unsigned int indexnum, void* indexdata, ID3D11Buffer** pIndexBuffer);
    static bool CreateVertexBuffer(unsigned int stride, unsigned int vertexnum, void* vertexdata,
                                   ID3D11Buffer** pVertexBuffer);
    static bool CreateVertexBufferWrite(unsigned int stride, unsigned int vertexnum, void* vertexdata,
                                        ID3D11Buffer** pVertexBuffer);

    static bool CreateConstantBuffer(unsigned int bytesize, ID3D11Buffer** pConstantBuffer);
    static bool CreateConstantBufferWrite(unsigned int bytesize, ID3D11Buffer** pConstantBuffer);
    static void SetLight(LIGHT light);
    static void SetMaterial(MATERIAL material);
    static void SetUV(float u, float v, float uw, float vh);

    // ブレンド ステート設定
    static void SetBlendState(int nBlendState) {
        if (nBlendState >= 0 && nBlendState < MAX_BLENDSTATE) {
            float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            m_pDeviceContext->OMSetBlendState(m_pBlendState[nBlendState], blendFactor, 0xffffffff);
        }
    }

    static void SetWireFrame(bool isWire);
};
