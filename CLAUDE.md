# MeltVerb — Claude Code プロジェクト設定

## プロジェクト概要

EarthQuaker Devices Avalanche Run にインスパイアされた空間系 VST3 プラグイン。
EMverb の Dattorro plate reverb を内部分解し、InputDiffuser + MeltDelay (3モード) +
ReverbTank + CrossFeed で唯一無二の空気感を生む。
48kHz ネイティブ動作、32-bit float。
JUCE フレームワークで開発。外部依存なしで自己完結する。

## JUCE ルール

- コードは常に JUCE 公式ドキュメント (https://docs.juce.com/) の最新仕様に準拠すること。
- 非推奨 (deprecated) の API は使用しないこと。
  - 例: `juce::Font::Font(float size)` → `juce::Font(juce::FontOptions{}.withHeight(size))`
  - 例: APVTS のパラメータ追加には `juce::ParameterID` を使う。
- `JuceHeader.h` の利用には `juce_generate_juce_header(<target>)` を CMakeLists.txt に記述。
- JUCE モジュールは `target_link_libraries` で `juce::juce_*` 形式で指定。
- MSVC ビルドでは `_USE_MATH_DEFINES` を定義して `M_PI` を有効に。

## CMake ルール

- `cmake_minimum_required` は 3.22 以上。
- プラグインターゲットには `juce_add_plugin()` を使い、`FORMATS` に AU VST3 Standalone。
- JUCE_DIR 環境変数でパスを指定: `add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)`

## 全般ルール

- C++17 以上。
- 生ポインタよりスマートポインタ優先。
- processBlock 内で new/delete 禁止。
- 1関数 50行以内、1ファイル 300行以内。超えたら分割を検討し報告。
- std::atomic または JUCE APVTS 経由でスレッドセーフを確保。

## ビルドコマンド

    cmake -B build
    cmake --build build/
    ctest --test-dir build/

## コミット前チェック

`cmake --build build/` が成功すること。

## ハンドオフワークフロー

- 仕様・設計・タスク: `.claude/handoff/`
- ビルドエラー: `.claude/build-errors.txt`
- エスカレーション: `.claude/escalation-report.md`

## コード修正の原則

仕様書で定義されたコードを尊重する。
修正は根拠がある場合のみ、最小限で。
詳細は `.claude/commands/build-fix.md` の指示に従う。

## DSP 実装で「やってはいけない」こと

### リアルタイム安全
- processBlock / process() 内での `new` / `delete` / `malloc` / `free` は絶対禁止。
- processBlock 内での `std::vector` の resize / push_back 禁止（ヒープ確保が走る）。
- processBlock 内での mutex ロック禁止（デッドロック・優先度逆転のリスク）。
- processBlock 内での `std::cout` / `DBG()` / ファイルI/O 禁止（ブロッキング）。
- 非リアルタイムスレッドから DSP 状態を直接書き込まないこと。必ず `std::atomic` または APVTS 経由で渡す。

### 数値安定性
- フィードバックループ内で gain > 1.0 になる組み合わせを許可しないこと。
  - feedback + crossFeed の合計を 0.95 以下に正規化する処理を必ず維持すること。
- 遅延バッファの読み出しインデックスがバッファサイズを超えないよう、必ず `& bufMask_` でラップすること。
- `std::clamp` や `saturate()` を外した場合は必ず代替の発散防止処理を入れること。
- デノーマル数（非正規化数）対策として `juce::ScopedNoDenormals` を processBlock 冒頭に置くこと。
- `std::tanh` はループ内での多用は CPU 負荷が高い。ループ内 saturate は最小限（Side A / Side B 各1回）に留めること。

### フィルタ設計
- フィードバックループ内の IIR フィルタ係数は安定性確認（極が単位円内）を必ず行うこと。
- SVF の `f` 係数は `std::min(f, 0.85f)` でクランプすること（0.85 超えで不安定になる）。
- 一次IIR の係数 `coeff` は `[0.0, 1.0)` の範囲を守ること。1.0 以上で発散する。

### JUCE API
- `juce::Font::Font(float size)` は deprecated。必ず `juce::FontOptions` を使うこと。
- `AudioParameterInt` を ComboBox にアタッチする場合、ComboBox の Item ID は
  パラメータの最小値+1 から始まるため、0-indexed パラメータ値と +1 のズレが生じる。
  `AudioParameterChoice` を使うか、ID のオフセットを明示的に管理すること。
- `SliderAttachment` / `ButtonAttachment` は対応する `Slider` / `Button` より
  **後に**破棄されなければならない（Attachment が先に破棄されるとクラッシュ）。
  宣言順序はメンバ変数でも `unique_ptr` でも、Attachment を Widget より後に置くこと。
- `AlertWindow` / `FileChooser` などモーダル UI は `enterModalState` ではなく
  `launchAsync` を使うこと（JUCE 7 以降 synchronous modal は非推奨）。
- `resized()` でコンポーネントの `setBounds()` に残り領域全体を渡さないこと。
  `btn.setBounds(col)` のように残り全域を割り当てると hitbox が意図しない範囲に広がり、
  他のコンポーネント（ComboBox 等）のクリックを横取りする。
  必ず `col.removeFromTop(h)` 等で明示的にサイズを切り出してから渡すこと。

### オーディオ品質
- リバーブのフィードバックループ内でハードクリップ（`std::clamp`）を使わないこと。
  必ず `saturate()` (tanh ベース) を使い、アナログ的な軟飽和で発散を防ぐこと。
- ループ外の最終出力段のみ `std::clamp` または tanh で二重保護してよい。
- allpass フィルタのカップリング係数 `kap` は `[0.0, 1.0)` の範囲を守ること。
  1.0 以上で不安定（発散）、負値でも可だが音質が変わるため意図を明記すること。
- サンプルレート変更時は必ず `prepare(newSampleRate)` を呼び、
  遅延バッファ・フィルタ状態を再初期化すること。

### 変更管理
- DSP パラメータのデフォルト値を変更する場合は、既存プリセット（.xml）との
  互換性が失われることを確認してから変更すること。
- `Parameters.h` のパラメータ ID 文字列は一度公開したら変更禁止
  （DAW のオートメーションレーンが切れる）。
- 既存テスト（`Tests/MeltEngineTests.cpp`）が全て通ることをコミット前に確認すること。
  `ctest --test-dir build/` でグリーンになること。
