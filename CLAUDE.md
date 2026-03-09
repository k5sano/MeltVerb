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
