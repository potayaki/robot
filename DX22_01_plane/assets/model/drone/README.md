# ホバードローンのモデル

このフォルダに `Drone.fbx` を置くと、ゲーム起動時に簡易的な立方体のドローンからFBX表示へ切り替わります。FBXが参照するテクスチャ画像も同じフォルダに置いてください。

表示の大きさ・向き・上下位置は `CFlyingEnemy.cpp` 冒頭の `DroneModelScale`、`DroneModelYawOffset`、`DroneModelOffsetY` などで調整します。モデルに画像が反映されない場合は `DroneOverrideTexture` に画像パスを指定できます。当たり判定の半径は `CFlyingEnemy.h` の `GetCollisionSphere()` で調整します。

FBXがない場合は従来の簡易ドローンを表示します。敵の移動、攻撃、被弾判定はどちらの表示でも共通です。
