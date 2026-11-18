/// @brief FBX素材の軸を保ち、DebugとReleaseでモデルの向きをそろえる。
#pragma once
#include <assimp/Importer.hpp>

inline void ConfigureFbxImport(Assimp::Importer& importer) {
    // ゲーム側の姿勢・骨行列はAssimp 5.2.5の軸を基準にしている。
    // マクロの有無で設定を省かず、両バージョンに同じ読込方針を指定する。
    // 5.2.5は未知の設定を無視し、6.xはFBXの軸による追加回転を停止する。
    importer.SetPropertyBool("AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION", true);
}
