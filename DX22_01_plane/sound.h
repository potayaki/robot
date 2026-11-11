/// @file sound.h
/// @brief WAV音声の読込、XAudio2による再生と停止。

#pragma once

#include <xaudio2.h>

// サウンドファイル
typedef enum {
    SOUND_LABEL_BGM000 = 0, // サンプルBGM
    //SOUND_LABEL_BGM001,		// サンプルBGM
    //SOUND_LABEL_SE000,		// サンプルSE
    //SOUND_LABEL_SE001,		// サンプルSE

    SOUND_LABEL_MAX,
} SOUND_LABEL;

class Sound {
private:
    // パラメータ構造体
    typedef struct {
        LPCSTR filename; // 音声ファイルまでのパスを設定
        bool bLoop; // trueでループ。通常BGMはtrue、SEはfalse。
    } PARAM;

    /// 音声ごとのファイル名とループ再生の設定。
    PARAM m_param[SOUND_LABEL_MAX] = {
        {"asset/BGM/sample000.wav", true}, // サンプルBGM（ループさせるのでtrue設定）
        //		{"asset/BGM/○○○.wav", true},	// サンプルBGM
        //		{"asset/SE/○○○.wav", false},  		// サンプルSE（ループしないのでfalse設定）
        //		{"asset/SE/○○○.wav", false},		// サンプルSE

    };

    /// 音声の読込・再生を管理するXAudio2エンジン。
    IXAudio2* m_pXAudio2 = NULL;
    /// 全音声をまとめて出力するマスターボイス。
    IXAudio2MasteringVoice* m_pMasteringVoice = NULL;
    /// 音声ごとの再生状態を持つソースボイス。
    IXAudio2SourceVoice* m_pSourceVoice[SOUND_LABEL_MAX];
    /// WAVフォーマット
    WAVEFORMATEXTENSIBLE m_wfx[SOUND_LABEL_MAX];
    /// 描画データをGPUへ送るバッファ。
    XAUDIO2_BUFFER m_buffer[SOUND_LABEL_MAX];
    /// WAVから読み込んだ音声サンプルの領域。
    BYTE* m_dataBuffer[SOUND_LABEL_MAX];

    HRESULT FindChunk(HANDLE, DWORD, DWORD&, DWORD&);
    HRESULT ReadChunkData(HANDLE, void*, DWORD, DWORD);

public:
    // ゲームループ開始前に呼び出すサウンドの初期化処理
    /// 使用前の状態と必要な資源を初期化する。
    HRESULT Init(void);

    // ゲームループ終了後に呼び出すサウンドの解放処理
    /// 使用した資源を解放して終了する。
    void Uninit(void);

    // 引数で指定したサウンドを再生する
    void Play(SOUND_LABEL label);

    // 引数で指定したサウンドを停止する
    void Stop(SOUND_LABEL label);

    // 引数で指定したサウンドの再生を再開する
    void Resume(SOUND_LABEL label);
};