# Cream Grit M — VST版ガイド

<a id="japanese"></a>

**Windows：動作確認済み ／ macOS：未テストのアルファ版**

drumlogue用Cream Grit MASTERをもとにした、PC/Mac用のオーディオエフェクトです。ローファイな質感、サンプラー風ピッチシフト、音の圧縮感と空間系をまとめて使えます。
VST3プラグインにはVST3対応ホストが必要です。Standaloneアプリも同梱しています。この移植はAI（Claude）が行っており、十分なテストはされていません。

[入手・ビルド用README](../vst/README.md) · [drumlogue版へ戻る](../README.md) · [English](#english)

## 対応形式と確認状況

| 環境 | 形式 | 状態 |
|---|---|---|
| Windows | VST3 / Standalone | **動作確認済み** |
| macOS | VST3 / AU / Standalone | **Alpha・未テスト** |

Windowsの動作確認は、すべてのDAW・OSバージョン・オーディオデバイスでの互換性を保証するものではありません。macOSはCIビルドが成功していますが、Mac実機での起動・音声処理・DAWへの読み込みは未確認です。Apple Silicon / IntelのUniversal、macOS 11以降という指定はビルド設定上の対象です。

## ダウンロード

**[VST版Release — v0.6.0 Alpha 1](https://github.com/ufotone/Cream-Grit/releases/tag/vst-v0.6.0-alpha.1)**

| 環境 | ダウンロード | 同梱形式 |
|---|---|---|
| Windows・動作確認済み | **[Windows版ZIPをダウンロード](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-Windows.zip)** | VST3 / Standalone exe |
| macOS・Alpha／未テスト | **[Mac版ZIPをダウンロード](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-macOS.zip)** | VST3 / AU / Standalone app |

リンクからZIPをダウンロードして展開してください。GitHubへのログインやActionsの操作は不要です。Mac版が未テストのためRelease全体をプレリリース扱いにしています。Windows版は動作確認済みです。drumlogue用の `.drmlgunit` はDAWでは読み込めません。[ソースビルド手順](../vst/README.md#開発者向けビルド)も利用できます。

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

Cream Grit M — VST guide

**Windows: tested / macOS: untested alpha**

Cream Grit M is a desktop audio effect based on the drumlogue MASTER edition. It combines lo-fi texture, sampler-style pitch shifting, compression and spatial effects. A VST3-capable host is required for the VST3 plug-in; standalone applications are also provided. This port was made using AI (Claude) and has not undergone thorough testing.

[Download and build README](../vst/README.md#english) · [Back to drumlogue documentation](../README.md#english) · [日本語](#japanese)

### Supported formats and test status

| Platform | Formats | Status |
|---|---|---|
| Windows | VST3 / Standalone | **Tested** |
| macOS | VST3 / AU / Standalone | **Alpha — untested** |

Windows testing does not guarantee compatibility with every DAW, OS version or audio device. macOS CI builds succeed, but launching, audio processing and DAW loading have not been verified on a Mac. Universal builds for Apple Silicon / Intel and macOS 11+ are build targets, not verified compatibility claims.

### Download

**[VST Release — v0.6.0 Alpha 1](https://github.com/ufotone/Cream-Grit/releases/tag/vst-v0.6.0-alpha.1)**

| Platform | Download | Contents |
|---|---|---|
| Windows — tested | **[Download Windows ZIP](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-Windows.zip)** | VST3 / Standalone exe |
| macOS — alpha, untested | **[Download Mac ZIP](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-macOS.zip)** | VST3 / AU / Standalone app |

Download and extract the ZIP directly. GitHub sign-in and GitHub Actions are not required. The release is marked as a prerelease because the Mac version is untested; Windows has been tested. The drumlogue `.drmlgunit` files cannot be loaded in a DAW. [Building from source](../vst/README.md#building-from-source) is also available.

### Installing on Windows

1. Close your DAW and extract the Windows ZIP.
2. Copy the entire `Cream Grit M.vst3` folder to `C:\Program Files\Common Files\VST3\`. Keep the package intact instead of copying only its internal files.
3. Launch your DAW and rescan VST3 plug-ins.
4. Insert **Cream Grit M** as an audio effect on a track or bus.

To use it without a DAW, launch `Cream Grit M.exe` and select input and output devices in its audio settings. The standalone application processes the selected audio input. It does not automatically capture everything playing through Windows.

### Installing on macOS (alpha — untested)

These are placement instructions. They have not been verified on a Mac.

| Format | Example location |
|---|---|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Cream Grit M.vst3` |
| AU | `~/Library/Audio/Plug-Ins/Components/Cream Grit M.component` |
| Standalone | Move the extracted `Cream Grit M.app` to an applications folder |

Rescan VST3 or AU in your DAW. The current CI package is ad-hoc signed and is not notarized by Apple. macOS security settings may restrict launching or loading. No DAW / Mac configuration is currently listed as verified.

VST3 locations follow the [Steinberg specification](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html).

### Getting started

1. Feed in audio, adjust GRIT for the amount of processed sound, and CLOCK for the sample-hold texture.
2. To use pitch processing, set PITCH away from zero and choose a P-MODE. SAMPLER uses short grains and non-interpolated reads.
3. Use COMP for full-band compression. This is separate from the bass-focused BODY control.
4. Add ECHO, CHORUS and REVERB as needed, then match output volume with LEVEL.

Defaults are P-MODE=SAMPLER, COMP=45% and PITCH=0. **PITCH=0 or P-MIX=0 suspends the pitch stage, so selecting SAMPLER alone does not apply its pitch-processing texture.** In SAMPLER mode, CLOCK also affects the virtual playback clock. P-SCOPE=HIGH protects the bass; FULL processes the entire pitch-stage input.

### Parameters

Ranges and defaults match the VST implementation. Hosts display the controls in the group order below.

| Name | Range (default) | Function |
|---|---|---|
| GRIT | 0–100% (65) | Amount of lo-fi processing mixed into the mids and highs |
| CLOCK | −24–0 semitones (0) | Virtual sample clock; also affects SAMPLER's hold clock |
| BITS | 6–16 (12) | Lo-fi quantization bit depth |
| TONE | 0–100% (55) | Brightness of the processed sound |
| DRIVE | 0–100% (30) | Mid/high saturation |
| SPLIT | 150–1,200 Hz (350) | Bass-protection crossover |
| RADIO | 0–100% (10) | Input-following FM-style noise; zero is OFF |
| BODY | 0–100% (40) | Bass compression and weight |
| COMP | 0–100% (45) | Stereo-linked full-band compressor; zero is OFF |
| PITCH | −36–+24 semitones (0) | Pitch control separate from CLOCK |
| P-MODE | CLEAN / SAMPLER / DRUMLOGUE (SAMPLER) | Pitch-processing algorithm |
| P-MIX | 0–100% (100) | Replacement amount from pre-pitch to shifted sound |
| P-SCOPE | HIGH / FULL (FULL) | HIGH protects bass; FULL processes the whole pitch input |
| ECHO | 0–100% (0) | Tempo-sync delay amount |
| NOTE | 9 divisions (1/8) | Delay note division |
| FEEDBACK | 0–85% (25) | Delay feedback amount |
| MBPM | HOST / 30–300 BPM (HOST) | Follow host tempo or set manual BPM |
| CHORUS | 0–100% (0) | Stereo chorus amount |
| C-RATE | 0.1–3.0 Hz (0.8) | Chorus modulation rate |
| C-DEPTH | 0–100% (40) | Chorus modulation depth |
| REVERB | 0–100% (0) | Stereo reverb amount |
| R-DECAY | 0.3–8.0 seconds (1.8) | Approximate reverb decay time |
| R-DAMP | 0–100% (60) | Reverb high-frequency damping; higher is darker |
| RING | 0–50% (0) | Amount of ring-modulated sound added to the original |
| RM-FREQ | OFF / 1–230 Hz (OFF) | Ring-modulator frequency; zero is OFF |
| PAN | 0–100% (0) | Width of random main-signal panning |
| P-RATE | 0.1–8.0 Hz (1.0) | Rate of selecting the next pan destination |
| P-WAVE | SINE / SQUARE (SINE) | Smooth movement / position switching |
| LEVEL | −24–+6 dB (0) | Processed output level |

NOTE divisions: 1/32, 1/16T, 1/16, 1/8T, 1/16D, 1/8, 1/4T, 1/8D, 1/4. T means triplet; D means dotted. MBPM=HOST falls back to 120 BPM when the host supplies no tempo. Set MBPM to 30–300 for manual tempo, including in standalone use.

RING is active when both RING and RM-FREQ are above zero. CHORUS, REVERB and ECHO each suspend when their amount is zero. Panning affects the main signal, leaving the echo and reverb returns in place.

#### Pitch modes

| Mode | Character |
|---|---|
| CLEAN | 40 ms grains with interpolation and filtering |
| SAMPLER | Short 22 ms grains, non-interpolated reads and holding at a virtual playback clock |
| DRUMLOGUE | The original 100 ms grain algorithm |

The VST version has three modes. This differs from the two-mode drumlogue backport preview and its fixed-value / ON-OFF controls.

### Presets

| Name | Character |
|---|---|
| 12bit | 12-bit lo-fi processing, COMP=45% |
| Down Dirt | CLOCK=−13, COMP=60% |
| Wide Space | Chorus, reverb and mono delay |
| Random Drift | Wide Space with random main-signal panning |

Select presets through your DAW's program selector; presentation varies by host. The plug-in uses JUCE's generic parameter editor.

[Source and build instructions](../vst/README.md#building-from-source) · [Original VST port PR #1](https://github.com/ufotone/Cream-Grit/pull/1) · [drumlogue documentation](../README.md#english)
