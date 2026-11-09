# Build / ビルド

Requires Docker and the [KORG logue-sdk](https://github.com/korginc/logue-sdk).
DockerとKORG logue-sdkを使用します。

SDK revision: `e78b16c6068b1c30fc072f2aebb8da5eef6c005c`

Copy both `src/cream-grit/` and `src/cream-grit-master/` into the SDK's `platform/drumlogue/` directory. The MASTER project shares the DELAY project's DSP and interface.
`src/cream-grit/`と`src/cream-grit-master/`を両方ともSDKの`platform/drumlogue/`へコピーします。MASTER版はDELAY版のDSPとインターフェースを共用します。

```powershell
docker run --rm --user 0 --entrypoint /bin/bash -v 'C:/path/to/logue-sdk/platform:/workspace' xiashj/logue-sdk:latest /workspace/drumlogue/cream-grit/build-sdk.sh
docker run --rm --user 0 --entrypoint /bin/bash -v 'C:/path/to/logue-sdk/platform:/workspace' xiashj/logue-sdk:latest /workspace/drumlogue/cream-grit-master/build-sdk.sh
```

Outputs / 生成物：

- `platform/drumlogue/cream-grit/cream_grit.drmlgunit`
- `platform/drumlogue/cream-grit-master/cream_grit_master.drmlgunit`

DSP checks can be run from a Visual Studio developer command prompt.
DSPの確認はVisual Studio開発者コマンドプロンプトから実行できます。

```bat
cl /nologo /std:c++14 /O2 /EHsc /W4 /I src\cream-grit tests\cream_test.cpp /Fe:cream_test.exe
cream_test.exe
```
