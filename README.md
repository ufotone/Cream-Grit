# Cream Grit

KORG drumlogue用のローファイ、ピッチシフト、テンポ同期ディレイFX。DELAYスロットで使用します。

## 日本語

### 導入

1. [最新版のcream_grit.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit.drmlgunit)をダウンロードします。
2. drumlogueのUSB DEVICE端子をPCに接続し、RECボタンを押しながら電源を入れます。
3. 表示されたUSBドライブの`Units/DelayFXs/`にファイルをコピーします。
4. PC側で安全に取り外し、本体のYESを押して起動します。
5. DELAYの一覧から**Cream Grit**を選び、対象パートのDELAY SENDを上げます。

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
| LEVEL | −24〜＋6 dB（0） | FXリターンの出力音量。 |
| RADIO | 0〜100%（10） | 入力に追従する薄いFM風ノイズ。0でオフ。 |
| ECHO | 0〜100%（0） | テンポ同期ディレイの戻し量。0で休止。 |
| NOTE | 9種類（1/8） | ディレイの音符間隔。 |
| FEEDBACK | 0〜85%（25） | ディレイの繰り返し量。 |
| PITCH | −36〜＋24半音（0） | CLOCKとは独立した音程操作。 |
| P-MIX | 0〜100%（100） | ピッチ前の音をピッチ後の音に置き換える割合。 |
| P-SCOPE | HIGH / FULL（FULL） | HIGHはSPLITで低域を保護。FULLはFXリターン全体をピッチ処理。 |

NOTE：1/32、1/16T、1/16、1/8T、1/16D、1/8、1/4T、1/8D、1/4。T＝三連、D＝付点。本体テンポの30〜300 BPMに同期し、最大2秒のモノラル反復音を返します。

PITCHが0、またはP-MIXが0ならピッチ処理を休止します。FULL・P-MIX 100%ではFXリターンを置き換えます。本体側の原音はセンド構成に従って並列に鳴ります。ピッチダウンでは前段LPFを弱め、ピッチアップでは折り返し対策を残しています。

## English

A lo-fi, pitch-shift and tempo-sync delay effect for the KORG drumlogue DELAY slot.

### Installation

1. Download the latest [cream_grit.drmlgunit](https://github.com/ufotone/Cream-Grit/releases/latest/download/cream_grit.drmlgunit).
2. Connect the drumlogue USB DEVICE port to your computer. Hold REC while powering on.
3. Copy the file into `Units/DelayFXs/` on the mounted USB drive.
4. Safely eject the drive, then press YES on the drumlogue to start.
5. Select **Cream Grit** in the DELAY effect list and raise DELAY SEND for the desired parts.

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
| LEVEL | −24–+6 dB (0) | Output level of the effect return. |
| RADIO | 0–100% (10) | Subtle FM-style noise that follows the input. Zero turns it off. |
| ECHO | 0–100% (0) | Tempo-sync delay return level. Zero suspends the delay. |
| NOTE | 9 divisions (1/8) | Delay note division. |
| FEEDBACK | 0–85% (25) | Delay feedback amount. |
| PITCH | −36–+24 semitones (0) | Musical pitch shift, independent of CLOCK. |
| P-MIX | 0–100% (100) | Crossfade from the pre-pitch sound to the pitch-shifted sound. |
| P-SCOPE | HIGH / FULL (FULL) | HIGH protects bass using SPLIT. FULL shifts the entire effect return. |

NOTE divisions: 1/32, 1/16T, 1/16, 1/8T, 1/16D, 1/8, 1/4T, 1/8D, 1/4. T = triplet; D = dotted. The mono delay follows the host tempo from 30 to 300 BPM, with a maximum length of two seconds.

PITCH 0 or P-MIX 0 suspends pitch processing. FULL with P-MIX 100% replaces the effect return; the drumlogue dry signal remains on its parallel send routing. Pitch-down uses a lighter input low-pass filter, while pitch-up retains anti-alias filtering.

[Build / ビルド](BUILD.md) · [MIT License](LICENSE) · [KORG BSD-3-Clause License](LICENSE-KORG-BSD3.txt)
