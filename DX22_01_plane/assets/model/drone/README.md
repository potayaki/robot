# ホバードローンのモデル

このフォルダに `Drone.fbx` を置くと、ゲーム起動時に簡易的な立方体のドローンからFBX表示へ切り替わります。FBXが参照するテクスチャ画像も同じフォルダに置いてください。

表示の大きさ・向き・上下位置は `CFlyingEnemy.cpp` 冒頭の `DroneModelScale`、`DroneModelYawOffset`、`DroneModelOffsetY` などで調整します。モデルに画像が反映されない場合は `DroneOverrideTexture` に画像パスを指定できます。当たり判定の半径は `CFlyingEnemy.h` の `GetCollisionSphere()` で調整します。

現在はMonstersのSwarm09を使用し、`DroneOverrideTexture` に `assets/model/drone/albedo.tga` を指定しています。FBX内には元の作者の環境の `Albedo.png` へのパスが残っているため、画像を置くだけでなく、この指定で色を割り当てます。

`DroneModelScale` は0.2（幅約28）、当たり判定は半径12です。`DroneModelLocalCenter` でSwarm09の頂点範囲の中心を指定し、表示の中心が敵の座標と一致するようにしています。別のモデルに変更する際は、拡大率だけでなくこの中心位置も合わせてください。

FBXがない場合は従来の簡易ドローンを表示します。敵の移動、攻撃、被弾判定はどちらの表示でも共通です。
