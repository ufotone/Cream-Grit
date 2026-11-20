# Cream Grit M — VST port

drumlogue用Cream GritのMASTER版をPC/Macへ移植したオーディオエフェクトです。ローファイ加工、ピッチシフト、コンプレッサー、テンポ同期ディレイ、コーラス、リバーブ、リング変調、ランダムパンを備えます。drumlogue本体は不要です。

**Windows：動作確認済み。macOS：未テストのアルファ版。** Windowsの確認結果は、すべてのDAW・オーディオデバイスでの互換性を保証するものではありません。macOSはCIビルドが成功していますが、実機での起動・音声処理・DAWへの読み込みは未確認です。

| 環境 | ビルド形式 | 動作確認 |
|---|---|---|
| Windows | VST3 / Standalone | 確認済み |
| macOS | VST3 / AU / Standalone | **Alpha・未テスト** |

[専用ページ：入手・導入・使い方・全パラメーター](../docs/VST.md) · [drumlogue版](../README.md) · [元のVSTポート PR #1](https://github.com/ufotone/Cream-Grit/pull/1)

## 入手

**[VST版Release — v0.6.0 Alpha 1](https://github.com/ufotone/Cream-Grit/releases/tag/vst-v0.6.0-alpha.1)**

- **[Windows版ZIPをダウンロード](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-Windows.zip)** — VST3 / Standalone、動作確認済み
- **[Mac版ZIPをダウンロード](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-macOS.zip)** — VST3 / AU / Standalone、Alpha・未テスト

ZIPをダウンロードして展開するだけで、GitHubへのログインやActionsの操作は不要です。Mac版が未テストのためRelease全体はプレリリース扱いです。drumlogue向けの `.drmlgunit` はPC用プラグインではありません。導入手順は[専用ページ](../docs/VST.md)をご覧ください。

## 開発者向けビルド

CMake 3.22以上、C++17対応コンパイラー、JUCE 8.0.10を使用します。WindowsではVisual StudioのC++開発環境、macOSではXcodeのコマンドライン開発環境が必要です。リポジトリーのルートで実行します。

```sh
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE vst/external/JUCE
cmake -S vst -B vst/build -DCMAKE_BUILD_TYPE=Release
cmake --build vst/build --config Release --parallel --target CreamGritM_All null_test_delay null_test_master
```

主な生成先は `vst/build/CreamGritM_artefacts/Release/` です。ジェネレーターによって構成名のサブフォルダーがない場合があります。CMake設定上、macOSは11以降を対象にApple Silicon / IntelのUniversalビルドを指定していますが、**その環境での動作は未確認です**。

null testは生成した `null_test_delay` と `null_test_master`（Windowsでは `.exe`）を実行します。比較はCOMP=0、P-MODE=DRUMLOGUE、RING=0で従来drumlogue DSPを基準に行います。48 kHzの一致と、ほかのサンプルレートでの有限値・出力範囲を検査します。CIテストの成功はmacOS実機での動作確認を意味しません。

### Windows専用Loopback FX Host（任意）

Windowsの再生音をWASAPI loopbackで取り込み、VST3を通して別の出力デバイスへ送る補助ホストです。通常のDAW利用には不要で、Releaseの配布ZIPには含まれません。

```sh
cmake --build vst/build --config Release --parallel --target LoopbackFxHost
```

取り込み元と処理後の出力先は別のデバイスを選びます。同じ出力を再び取り込むとループになります。ホストのBPMを使うときは、プラグインのMBPMをHOSTにします。この補助ホスト自体の動作確認範囲は、VST版のWindows動作確認とは別です。

## English

Cream Grit M is a desktop port of the drumlogue MASTER effect, with lo-fi processing, pitch shifting, compression, tempo-sync delay, chorus, reverb, ring modulation and random panning. No drumlogue hardware is required.

**Windows: tested. macOS: untested alpha.** Windows testing does not guarantee compatibility with every DAW or audio device. macOS CI builds succeed, but launching, audio processing and DAW loading have not been verified on a Mac.

| Platform | Build formats | Test status |
|---|---|---|
| Windows | VST3 / Standalone | Tested |
| macOS | VST3 / AU / Standalone | **Alpha — untested** |

[Dedicated guide: download, installation, usage and all controls](../docs/VST.md#english) · [drumlogue edition](../README.md#english) · [Original VST port PR #1](https://github.com/ufotone/Cream-Grit/pull/1)

### Download

**[VST Release — v0.6.0 Alpha 1](https://github.com/ufotone/Cream-Grit/releases/tag/vst-v0.6.0-alpha.1)**

- **[Download Windows ZIP](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-Windows.zip)** — VST3 / Standalone; tested
- **[Download Mac ZIP](https://github.com/ufotone/Cream-Grit/releases/download/vst-v0.6.0-alpha.1/CreamGritM-macOS.zip)** — VST3 / AU / Standalone; alpha, untested

Download and extract the ZIP. GitHub sign-in and GitHub Actions are not required. The release is marked as a prerelease because the Mac version is untested. The drumlogue `.drmlgunit` files are not desktop plug-ins. See the [dedicated guide](../docs/VST.md#english) for installation.

### Building from source

Use CMake 3.22+, a C++17 compiler and JUCE 8.0.10. Windows needs a Visual Studio C++ development environment; macOS needs Xcode's command-line development tools. Run these commands from the repository root.

```sh
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE vst/external/JUCE
cmake -S vst -B vst/build -DCMAKE_BUILD_TYPE=Release
cmake --build vst/build --config Release --parallel --target CreamGritM_All null_test_delay null_test_master
```

The main output location is `vst/build/CreamGritM_artefacts/Release/`. Some generators omit the configuration subfolder. CMake targets macOS 11+ and Universal Apple Silicon / Intel builds, but **runtime compatibility on those systems remains unverified**.

Run the generated `null_test_delay` and `null_test_master` executables (`.exe` on Windows). They compare against the original drumlogue DSP with COMP=0, P-MODE=DRUMLOGUE and RING=0, checking the 48 kHz match plus finite, bounded output at other sample rates. Passing CI tests does not establish successful operation on a Mac.

#### Optional Windows-only Loopback FX Host

This helper captures a Windows playback device using WASAPI loopback, processes it through VST3 and sends it to a different output device. It is not needed for ordinary DAW use and is not included in the release ZIPs.

```sh
cmake --build vst/build --config Release --parallel --target LoopbackFxHost
```

Choose different capture-source and processed-output devices. Capturing the processed output again creates a loop. Set the plug-in's MBPM to HOST to use the helper host's BPM. The helper host's test coverage is separate from the Windows VST test status.

[MIT License](../LICENSE) · [KORG BSD-3-Clause License](../LICENSE-KORG-BSD3.txt)
