# Cream Grit M — VST port

drumlogue用Cream GritのMASTER版をPC/Macへ移植したオーディオエフェクトです。ローファイ加工、ピッチシフト、コンプレッサー、テンポ同期ディレイ、コーラス、リバーブ、リング変調、ランダムパンを備えます。drumlogue本体は不要です。

**Windows：動作確認済み。macOS：未テストのアルファ版。** Windowsの確認結果は、すべてのDAW・オーディオデバイスでの互換性を保証するものではありません。macOSはCIビルドが成功していますが、実機での起動・音声処理・DAWへの読み込みは未確認です。

| 環境 | ビルド形式 | 動作確認 |
|---|---|---|
| Windows | VST3 / Standalone | 確認済み |
| macOS | VST3 / AU / Standalone | **Alpha・未テスト** |

[専用ページ：入手・導入・使い方・全パラメーター](../docs/VST.md) · [drumlogue版](../README.md) · [元のVSTポート PR #1](https://github.com/ufotone/Cream-Grit/pull/1)

## 入手

[GitHub Actions — VST build](https://github.com/ufotone/Cream-Grit/actions/workflows/vst-build.yml) の成功したmainブランチの実行を開き、ArtifactsからOS別の成果物をダウンロードします。GitHubへのログインが必要です。

- Windows：`CreamGritM-Windows` → 中の `CreamGritM-Windows.zip`
- macOS（Alpha）：`CreamGritM-macOS` → 中の `CreamGritM-macOS.zip`

[PR #1マージ後の成功ビルド](https://github.com/ufotone/Cream-Grit/actions/runs/44666525995)でも入手できます。Actionsの成果物には保存期限があります。期限切れの場合は、利用可能な成功ビルドまたは下記のソースビルドを利用してください。既存のdrumlogue向けReleaseにある `.drmlgunit` はPC用プラグインではありません。

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

Windowsの再生音をWASAPI loopbackで取り込み、VST3を通して別の出力デバイスへ送る補助ホストです。通常のDAW利用には不要で、上記Actionsの配布ZIPには含まれません。

```sh
cmake --build vst/build --config Release --parallel --target LoopbackFxHost
```

取り込み元と処理後の出力先は別のデバイスを選びます。同じ出力を再び取り込むとループになります。ホストのBPMを使うときは、プラグインのMBPMをHOSTにします。この補助ホスト自体の動作確認範囲は、VST版のWindows動作確認とは別です。

## English

Cream Grit M is the desktop port of the drumlogue MASTER effect. Windows has been tested by the maintainer. **macOS is an untested alpha**: CI builds succeed, but launching, audio processing and DAW loading have not been verified on a Mac.

Windows builds provide VST3 and Standalone; macOS builds provide VST3, AU and Standalone. Download OS-specific artifacts from [VST build](https://github.com/ufotone/Cream-Grit/actions/workflows/vst-build.yml); sign-in is required and artifacts expire. See the [dedicated guide](../docs/VST.md#english) for installation and controls.

[MIT License](../LICENSE) · [KORG BSD-3-Clause License](../LICENSE-KORG-BSD3.txt)
