# Cream Grit M — VST版ガイド

**Windows：動作確認済み ／ macOS：未テストのアルファ版**

drumlogue用Cream Grit MASTERをもとにした、PC/Mac用のオーディオエフェクトです。ローファイな質感、サンプラー風ピッチシフト、音の圧縮感と空間系をまとめて使えます。
ＶＳＴエフェクトをホストできるソフトウェアが必要です。　この移植はAI(Claude)が行っており十分なテストはされていません。 

[入手・ビルド用README](../vst/README.md) · [drumlogue版へ戻る](../README.md) · [English](#english)

## 対応形式と確認状況

| 環境 | 形式 | 状態 |
|---|---|---|
| Windows | VST3 / Standalone | **動作確認済み** |
| macOS | VST3 / AU / Standalone | **Alpha・未テスト** |

Windowsの動作確認は、すべてのDAW・OSバージョン・オーディオデバイスでの互換性を保証するものではありません。macOSはCIビルドが成功していますが、Mac実機での起動・音声処理・DAWへの読み込みは未確認です。Apple Silicon / IntelのUniversal、macOS 11以降という指定はビルド設定上の対象です。

## ダウンロード

[GitHub Actions — VST build](https://github.com/ufotone/Cream-Grit/actions/workflows/vst-build.yml)で成功したmainブランチの実行を開き、ページ下部のArtifactsを選びます。ダウンロードにはGitHubへのログインが必要です。

| Artifactsの名前 | 中にあるZIP | 同梱形式 |
|---|---|---|
| CreamGritM-Windows | CreamGritM-Windows.zip | VST3 / Standalone exe |
| CreamGritM-macOS | CreamGritM-macOS.zip | VST3 / AU / Standalone app（すべてAlpha） |

[PR #1マージ後の成功ビルド](https://github.com/ufotone/Cream-Grit/actions/runs/44666525995)も利用できます。ArtifactsのZIPを展開し、さらに中のOS別ZIPを展開してください。保存期限切れの場合は、利用可能な成功ビルドか[ソースビルド](../vst/README.md#開発者向けビルド)を使います。drumlogue用Releaseの `.drmlgunit` はDAWでは読み込めません。

## Windowsへの導入

1. DAWを終了し、Windows版のZIPを展開します。
2. `Cream Grit M.vst3` をフォルダーごと `C:\Program Files\Common Files\VST3\` にコピーします。内部のファイルだけを取り出さず、パッケージ全体をコピーしてください。
3. DAWを起動し、VST3プラグインを再スキャンします。
4. オーディオトラックやバスのエフェクトとして **Cream Grit M** を挿入します。

DAWを使わない場合は `Cream Grit M.exe` を起動し、オーディオ設定で入力・出力デバイスを選びます。Standaloneは選択した音声入力を処理します。Windowsで再生しているすべての音を自動で取り込むものではありません。

## macOSへの導入（Alpha・未テスト）

以下は配置手順です。Mac実機での動作確認は行っていません。

| 形式 | 配置例 |
|---|---|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Cream Grit M.vst3` |
| AU | `~/Library/Audio/Plug-Ins/Components/Cream Grit M.component` |
| Standalone | 展開した `Cream Grit M.app` をアプリケーション用フォルダーへ配置 |

DAWでVST3またはAUを再スキャンします。現在のCIパッケージはアドホック署名で、Appleの公証は行っていません。macOSのセキュリティ設定によって起動や読み込みが制限される場合があります。読み込みに成功したDAWやMacの構成は、現時点では確認済みとして案内していません。

VST3の配置先は[Steinbergの仕様](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html)に従っています。

## 最初の音づくり

1. 音声を入力し、GRITで加工音の量、CLOCKでサンプル保持の粗さを調整します。
2. ピッチを使う場合はPITCHを0以外にし、P-MODEを選びます。SAMPLERは短いグレインと補間しない読み出しによる質感です。
3. COMPで全帯域の圧縮感を調整します。低域用のBODYとは別の処理です。
4. ECHO、CHORUS、REVERBを必要な分だけ加え、LEVELで出力音量を合わせます。

初期値はP-MODE=SAMPLER、COMP=45%、PITCH=0です。**PITCH=0またはP-MIX=0ではピッチ段が休止するため、SAMPLERを選ぶだけではそのピッチ処理の質感は掛かりません。** SAMPLERではCLOCKが仮想再生クロックにも作用します。P-SCOPE=HIGHで低域を保護し、FULLではピッチ段への入力全体を処理します。

## パラメーター

範囲と初期値はVST版の実装に対応しています。ホストには下記のグループ順で表示されます。

| 名前 | 範囲（初期値） | 働き |
|---|---|---|
| GRIT | 0–100%（65） | 中高域へローファイ加工を混ぜる量 |
| CLOCK | −24–0半音（0） | 仮想サンプルクロック。SAMPLERの保持クロックにも作用 |
| BITS | 6–16（12） | ローファイ加工の量子化ビット数 |
| TONE | 0–100%（55） | 加工音の明るさ |
| DRIVE | 0–100%（30） | 中高域の飽和感 |
| SPLIT | 150–1,200 Hz（350） | 低域保護の境界 |
| RADIO | 0–100%（10） | 入力音量に追従するFM風ノイズ。0でOFF |
| BODY | 0–100%（40） | 低域の圧縮感と厚み |
| COMP | 0–100%（45） | ステレオ連動の全帯域コンプレッサー。0でOFF |
| PITCH | −36–＋24半音（0） | CLOCKとは別の音程操作 |
| P-MODE | CLEAN / SAMPLER / DRUMLOGUE（SAMPLER） | ピッチ処理方式 |
| P-MIX | 0–100%（100） | ピッチ前からピッチ後への置き換え量 |
| P-SCOPE | HIGH / FULL（FULL） | HIGHは低域を保護、FULLはピッチ入力全体を処理 |
| ECHO | 0–100%（0） | テンポ同期ディレイの量 |
| NOTE | 9種類（1/8） | ディレイの音符間隔 |
| FEEDBACK | 0–85%（25） | ディレイの繰り返し量 |
| MBPM | HOST / 30–300 BPM（HOST） | ホスト追従または手動テンポ |
| CHORUS | 0–100%（0） | ステレオコーラスの量 |
| C-RATE | 0.1–3.0 Hz（0.8） | コーラスの速さ |
| C-DEPTH | 0–100%（40） | コーラスの深さ |
| REVERB | 0–100%（0） | ステレオ残響の量 |
| R-DECAY | 0.3–8.0秒（1.8） | 残響の減衰時間の目安 |
| R-DAMP | 0–100%（60） | 残響の高域減衰。高いほど暗い |
| RING | 0–50%（0） | 原音に加えるリング変調音の量 |
| RM-FREQ | OFF / 1–230 Hz（OFF） | リング変調の周波数。0はOFF |
| PAN | 0–100%（0） | メイン音のランダムパンの幅 |
| P-RATE | 0.1–8.0 Hz（1.0） | パンの次の目的位置を選ぶ周期 |
| P-WAVE | SINE / SQUARE（SINE） | 滑らかな移動 / 位置の切り替え |
| LEVEL | −24–＋6 dB（0） | 処理後の出力音量 |

NOTE：1/32、1/16T、1/16、1/8T、1/16D、1/8、1/4T、1/8D、1/4。Tは三連、Dは付点です。MBPM=HOSTでホストがテンポを返さない場合は120 BPMを使います。Standaloneなどで手動指定したい場合はMBPMを30–300にします。

RINGはRINGとRM-FREQの両方を0以外にすると有効です。CHORUS、REVERB、ECHOはそれぞれ量0で休止します。パンはメイン音に作用し、ディレイとリバーブの戻り音には作用しません。

### ピッチモード

| モード | 特徴 |
|---|---|
| CLEAN | 40 msのグレインと補間・フィルターを使う方式 |
| SAMPLER | 22 msの短いグレイン、補間しない読み出し、仮想再生クロックでの保持 |
| DRUMLOGUE | 従来の100 msグレインを使う方式 |

VST版には3モードがあります。drumlogue向け逆移植試作版の2モード構成や、各項目の固定値・ON/OFF化とは異なります。

## プリセット

| 名前 | 概要 |
|---|---|
| 12bit | 12bitのローファイ加工、COMP=45% |
| Down Dirt | CLOCK=−13、COMP=60% |
| Wide Space | コーラス、リバーブ、モノラルディレイ |
| Random Drift | Wide Spaceにメイン音のランダムパンを追加 |

プリセットはDAWのプログラム選択から呼び出します。表示方法はホストによって異なります。プラグインの画面はJUCEの汎用パラメーターエディターです。

## English

Cream Grit M is a desktop audio effect based on the drumlogue MASTER edition. It combines lo-fi processing, pitch shifting, compression, tempo-sync echo, chorus, reverb, ring modulation and random panning. No drumlogue hardware is required.

**Windows has been tested by the maintainer. macOS is an untested alpha.** macOS CI builds succeed, but launching, audio processing and DAW loading have not been verified on a Mac. Universal / macOS 11+ are build targets, not verified compatibility claims.

Download OS-specific artifacts from [VST build](https://github.com/ufotone/Cream-Grit/actions/workflows/vst-build.yml), then extract both the artifact ZIP and the platform ZIP inside it. GitHub sign-in is required; artifacts expire.

- Windows: copy the entire `Cream Grit M.vst3` package to `C:\Program Files\Common Files\VST3\`, rescan in your DAW, and insert **Cream Grit M** as an audio effect. The standalone `.exe` processes the selected audio input.
- macOS (alpha): place VST3 in `~/Library/Audio/Plug-Ins/VST3/`, AU in `~/Library/Audio/Plug-Ins/Components/`, or launch the standalone `.app`. The CI package uses ad-hoc signing and is not notarized.
- VST pitch modes are **CLEAN / SAMPLER / DRUMLOGUE**. Default COMP is 45%; PITCH defaults to 0, which suspends the pitch stage. Set PITCH away from zero to hear the SAMPLER pitch processing.
- Set MBPM to HOST for host tempo, or 30–300 for manual BPM. Missing host tempo falls back to 120 BPM.
- RING requires both RING and RM-FREQ above zero. Unlike the drumlogue backport preview, VST controls retain their full ranges.

[Source and build instructions](../vst/README.md#開発者向けビルド) · [Original VST port PR #1](https://github.com/ufotone/Cream-Grit/pull/1) · [drumlogue documentation](../README.md)
