# Cream Grit

KORG drumlogue用のローファイ、ピッチシフト、テンポ同期ディレイFX。DELAY版とMASTER版があります。

## 日本語

### 導入

| 版 | ダウンロード | コピー先 | 本体の表示名 |
|---|---|---|---|
| DELAY | [cream_grit.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit.drmlgunit) | `Units/DelayFXs/` | Cream Grit |
| MASTER | [cream_grit_master.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit_master.drmlgunit) | `Units/MasterFXs/` | Cream Grit M |
| MASTER試作版 | [v0.5.0-beta.1](https://github.com/ufotone/Cream-Grit/releases/download/v0.5.0-beta.1/cream_grit_master.drmlgunit) | `Units/MasterFXs/` | Cream Grit M |

1. 使用する版をダウンロードします。DELAY版とMASTER版は同時に導入できます。MASTERの安定版と試作版は入れ替えて使います。
2. drumlogueのUSB DEVICE端子をPCに接続し、RECボタンを押しながら電源を入れます。
3. 表示されたUSBドライブの、上記のコピー先にファイルを入れます。
4. PC側で安全に取り外し、本体のYESを押して起動します。
5. DELAY版はDELAYの一覧から**Cream Grit**を選び、対象パートのDELAY SENDを上げます。MASTER版はMASTERの一覧から**Cream Grit M**を選びます。

### Audio Inで使う（MASTER版）

1. 外部機器をAUDIO IN端子に接続します。
2. SHIFTを押しながらボタン1（AUDIO IN）を押し、`ROUTE`を`MASTER`にします。
3. `LEVEL`で入力音量を調整します。Cream Gritの`LEVEL`は処理後の出力音量です。

MASTER版はインサート処理なので、Audio Inの音がそのまま処理音に置き換わります。外部音声だけに掛ける場合は、内蔵各パートの`MASTER`を`BYPASS`にします。DELAY/REVERBの戻り音も処理から外したい場合は、それぞれの`RETURN`を`POST`にします。

### パラメーター

| 名前 | 範囲（初期値） | 説明 |
|---|---|---|
| GRIT | 0〜100%（65） | 中高域のローファイ加工を混ぜる量。 |
| CLOCK | −24〜0（0） | 仮想サンプルクロック。下げると粗さが変化。音程は変えません。 |
| BODY | 0〜100%（40） | 低域の圧縮感と厚み。 |
| TONE | 0〜100%（55） | ローファイ加工音の明るさ。 |
| SPLIT | 150〜1,200 Hz（350） | ローファイ処理とHIGHピッチ処理から低域を保護する境界。 |
| BITS | 6〜16（12） | ローファイ加工の量子化ビット数。 |
| DRIVE | 0〜100%（30） | 中高域の飽和感。 |
| LEVEL | −24〜＋6 dB（0） | 処理音の出力音量。 |
| RADIO | 0〜100%（10） | 入力に追従する薄いFM風ノイズ。0でオフ。 |
| ECHO | 0〜100%（0） | テンポ同期ディレイの戻し量。0で休止。 |
| NOTE | 9種類（1/8） | ディレイの音符間隔。 |
| FEEDBACK | 0〜85%（25） | ディレイの繰り返し量。 |
| PITCH | −36〜＋24半音（0） | CLOCKとは独立した音程操作。 |
| P-MIX | 0〜100%（100） | ピッチ前の音をピッチ後の音に置き換える割合。 |
| P-SCOPE | HIGH / FULL（FULL） | HIGHはSPLITで低域を保護。FULLはFXリターン全体をピッチ処理。 |

NOTE：1/32、1/16T、1/16、1/8T、1/16D、1/8、1/4T、1/8D、1/4。T＝三連、D＝付点。本体テンポの30〜300 BPMに同期し、最大2秒の反復音を返します。DELAY版のディレイはモノラルです。

### MASTER試作版の追加パラメーター

v0.5.0-beta.1では、コーラスの後にディレイとリバーブを並列で加えます。最初の2プリセットは追加機能をオフ、ディレイをMONOにしています。**Wide Space**プリセットではコーラス・リバーブ・ステレオディレイをまとめて使えます。

| 名前 | 範囲（初期値） | 説明 |
|---|---|---|
| E-MODE | MONO / STEREO（MONO） | ディレイ反復音の形式。STEREOは左右独立。切り替え時は反復音を短くフェードしてリセット。 |
| CHORUS | 0〜100%（0） | 原音に加えるステレオ変調音の量。0で休止。 |
| C-RATE | 0.1〜3.0 Hz（0.8） | コーラスの揺れる速さ。 |
| C-DEPTH | 0〜100%（40） | コーラスの揺れる深さ。 |
| REVERB | 0〜100%（0） | ステレオ残響の量。0で休止。 |
| R-DECAY | 0.3〜8.0秒（1.8） | 残響の減衰時間の目安。 |
| R-DAMP | 0〜100%（60） | 残響の高域減衰。上げると暗くなります。 |

比較用にステレオディレイのバッファも固定確保するため、MONOでもメモリ容量は変わりません。MONOでは右側のディレイ処理を休止します。

[比較WAV](https://github.com/ufotone/Cream-Grit/releases/download/v0.5.0-beta.1/preview-space.wav)：基本音 → コーラス → リバーブ → MONOディレイ → STEREOディレイ → 全部＋MONO → 全部＋STEREO。各区間約8.2秒、間隔0.35秒。

PITCHが0、またはP-MIXが0ならピッチ処理を休止します。FULL・P-MIX 100%では処理対象の音全体をピッチ音に置き換えます。DELAY版では本体側の原音が並列に鳴り、MASTER版ではMASTERに送った音を直接処理します。ピッチダウンでは前段LPFを弱め、ピッチアップでは折り返し対策。

## English

A lo-fi, pitch-shift and tempo-sync delay effect for KORG drumlogue, available for the DELAY and MASTER slots.

### Installation

| Edition | Download | Destination | Display name |
|---|---|---|---|
| DELAY | [cream_grit.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit.drmlgunit) | `Units/DelayFXs/` | Cream Grit |
| MASTER | [cream_grit_master.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit_master.drmlgunit) | `Units/MasterFXs/` | Cream Grit M |
| MASTER preview | [v0.5.0-beta.1](https://github.com/ufotone/Cream-Grit/releases/download/v0.5.0-beta.1/cream_grit_master.drmlgunit) | `Units/MasterFXs/` | Cream Grit M |

1. Download the desired edition. DELAY and MASTER can be installed together; choose either the stable or preview MASTER build.
2. Connect the drumlogue USB DEVICE port to your computer. Hold REC while powering on.
3. Copy the file into the corresponding destination on the mounted USB drive.
4. Safely eject the drive, then press YES on the drumlogue to start.
5. For DELAY, select **Cream Grit** and raise DELAY SEND for the desired parts. For MASTER, select **Cream Grit M** in the MASTER effect list.

### Audio In with the MASTER edition

1. Connect the external source to AUDIO IN.
2. Hold SHIFT and press button 1 (AUDIO IN), then set `ROUTE` to `MASTER`.
3. Adjust the input `LEVEL`. Cream Grit's own `LEVEL` controls the processed output.

The MASTER edition processes Audio In as an insert, replacing its input with the processed sound. To leave internal parts unprocessed, set their `MASTER` routing to `BYPASS`. Set DELAY/REVERB `RETURN` to `POST` if their returns should also bypass the master effect.

### Parameters

| Name | Range (default) | Description |
|---|---|---|
| GRIT | 0–100% (65) | Amount of lo-fi processing in the mid and high frequencies. |
| CLOCK | −24–0 (0) | Virtual sample clock. Lower values change the grit, not the musical pitch. |
| BODY | 0–100% (40) | Low-frequency compression and body. |
| TONE | 0–100% (55) | Brightness of the lo-fi path. |
| SPLIT | 150–1,200 Hz (350) | Boundary that protects bass from lo-fi processing and HIGH-scope pitch changes. |
| BITS | 6–16 (12) | Quantization bit depth of the lo-fi path. |
| DRIVE | 0–100% (30) | Saturation in the mid and high frequencies. |
| LEVEL | −24–+6 dB (0) | Processed output level. |
| RADIO | 0–100% (10) | Subtle FM-style noise that follows the input. Zero turns it off. |
| ECHO | 0–100% (0) | Tempo-sync delay return level. Zero suspends the delay. |
| NOTE | 9 divisions (1/8) | Delay note division. |
| FEEDBACK | 0–85% (25) | Delay feedback amount. |
| PITCH | −36–+24 semitones (0) | Musical pitch shift, independent of CLOCK. |
| P-MIX | 0–100% (100) | Crossfade from the pre-pitch sound to the pitch-shifted sound. |
| P-SCOPE | HIGH / FULL (FULL) | HIGH protects bass using SPLIT. FULL shifts the entire effect return. |

NOTE divisions: 1/32, 1/16T, 1/16, 1/8T, 1/16D, 1/8, 1/4T, 1/8D, 1/4. T = triplet; D = dotted. The delay follows the host tempo from 30 to 300 BPM, with a maximum length of two seconds. The DELAY edition's echo remains mono.

### Additional MASTER preview parameters

In v0.5.0-beta.1, chorus feeds the parallel echo and reverb paths. The first two presets keep the new effects off and echo in MONO. **Wide Space** enables chorus, reverb and stereo echo together.

| Name | Range (default) | Description |
|---|---|---|
| E-MODE | MONO / STEREO (MONO) | Echo return format. STEREO uses independent channels. Changing modes briefly fades and resets the echo tail. |
| CHORUS | 0–100% (0) | Added stereo modulation level. Zero suspends it. |
| C-RATE | 0.1–3.0 Hz (0.8) | Chorus modulation rate. |
| C-DEPTH | 0–100% (40) | Chorus modulation depth. |
| REVERB | 0–100% (0) | Stereo reverb return level. Zero suspends it. |
| R-DECAY | 0.3–8.0 seconds (1.8) | Nominal tail decay time. |
| R-DAMP | 0–100% (60) | High-frequency damping. Higher values darken the tail. |

The preview reserves both echo buffers even in MONO mode for comparison. MONO suspends the right echo processor but does not reduce the fixed memory allocation.

[Preview WAV](https://github.com/ufotone/Cream-Grit/releases/download/v0.5.0-beta.1/preview-space.wav): base → chorus → reverb → mono echo → stereo echo → all with mono echo → all with stereo echo. Each section is approximately 8.2 seconds, separated by 0.35 seconds.

PITCH 0 or P-MIX 0 suspends pitch processing. FULL with P-MIX 100% replaces the entire processed signal with the shifted sound. The DELAY edition retains the drumlogue's parallel dry signal; the MASTER edition directly processes signals routed to MASTER. Pitch-down uses a lighter input low-pass filter, while pitch-up retains anti-alias filtering.

[Build / ビルド](BUILD.md) · [MIT License](LICENSE) · [KORG BSD-3-Clause License](LICENSE-KORG-BSD3.txt)
