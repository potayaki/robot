# プレイヤー用ロボット

`Retro_UnderWaterAssets_Freebies_FBX/Mech` の素材をこのフォルダへコピーして使用しています。
本体の `SK_ISO_Mech.fbx` は `Player.fbx` に名前を変更しています。

## 操作とアニメーション

| 操作 | 使用するアニメーション |
| --- | --- |
| 停止 | `A_ISO_Mech_Idle_01.fbx` |
| Wで前進 | `A_ISO_Mech_WalkForward_01.fbx` |
| Sで後退 | `A_ISO_Mech_WalkBackward_01.fbx` |
| Aで左移動 | `A_ISO_Mech_Walk_L_01.fbx` |
| Dで右移動 | `A_ISO_Mech_Walk_R_01.fbx` |

機体はカメラの前方を向き、後退・横移動でもその向きを保ちます。
斜め移動では前進・後退のアニメーションを優先します。
方向を切り替えたときは0.15秒で姿勢を補間し、回転にはSlerpを使います。
再生時間は `GameTime::DeltaSeconds()` で進めるため、FPSによらず同じ速度です。
移動速度のスキルに合わせて歩行の再生速度も上がります。
壁に止められた場合や空中では待機アニメーションを使います。

Debugビルドの `Skill Debug → ロボットのアニメーション → Animation preview` で、
移動せずに各モーションを確認できます。`Automatic` で通常操作に戻ります。
ゲームを一時停止するとアニメーションも停止します。

## 大きさ・描画の調整

- `PlayerRobotModel.cpp` の `RobotScale = 0.025f` が表示倍率です。高さ約20、脚を含む幅約27にしています。
- 素材の足元がY=0なので、表示位置をプレイヤー中心から当たり判定の半径分だけ下げています。
- 素材の正面は-Zのため、描画時にY軸へ180度回してゲームの正面と合わせています。
- FBX内のテクスチャ参照先が配布画像名と異なるため、`Textures/T_Mech_Base_Color.png` を直接読み込みます。
- FBX内にあるLOD0・LOD1を重ねて描かず、詳細版のLOD0だけを描きます。

`RobotAnimationRig` が骨格と別ファイルのアニメーションを読み込み、
`PlayerRobotModel` と `shader/playerRobotVS.hlsl` が骨の動きを頂点へ反映します。
この素材は各頂点が1本の骨に固定されているため、その構造に合わせた専用の実装です。
他のモデルを使う場合は、骨名・LOD名・頂点のウェイト構造を確認してください。

## 保管している追加素材

`Death`・`Landing` のアニメーションと、Normal・Roughness・Metallic・Emissiveの画像もコピーしています。
現在の歩行表示では、これらは使用していません。
実際のFBXを使った読み込み・骨格・FPSの確認は `tests/RobotAnimationTests.cpp` にあります。
