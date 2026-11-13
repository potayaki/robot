# タイトル用ジオラマ

`TitleScene.cpp` がこのフォルダのモデルを読み込み、キャラクターなしの小さな戦場を描画します。

- 足場: `floating-island`。横幅20、縦方向0.43倍に整えています。
- 岩・植物: `stylizednaturelite_fbx`。岩は大小を混ぜ、二つの瓦礫の山に配置しています。
- 草: `grass_patch_3_lods`。ノード変換後の最も詳細なメッシュ1個だけを使い、LODを重ねません。
- 木片: 同梱の `SM_Branch_01.fbx`。
- 金属片: `urban_shanty_town_sheet_metal_corrugated_03_xmufdce_low`。

中央は後からPlayerとEnemyを配置できるよう空けています。小物の高さは足場の三角形から求めています。
足場と小物は共通の行列で180秒に1周回転し、見下ろすカメラと照明は固定です。
開始ボタンは画面下部に表示します。

配置・サイズは `Diorama` コンストラクタの `place(...)` と `clumps`、回転速度は `TitleScene::Update()`、
視点と画面内の大きさは `Diorama::Draw()` で調整できます。
照明と葉の透過抜きは `shader/titleDioramaPS.hlsl`、背景は `shader/titleBackdropPS.hlsl` です。
岩の下には簡易的な接地影を置いています（リアルタイムの投影影ではありません）。

FBXの外部画像パスに依存せず、配布されたBaseMap / Albedoを明示的に割り当てます。
`dirty-stones-pile` は高密度のため、`landmine` は今回の構成に不要なため読み込みません。
元のモデル・画像は変更していません。
