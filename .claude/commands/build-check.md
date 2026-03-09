# /build-check — ビルド＆テスト実行

以下のコマンドを順に実行し、結果を報告すること:

1. `cmake -B build` (初回 or CMakeLists.txt 変更時)
2. `cmake --build build/`
3. `ctest --test-dir build/ --output-on-failure`

エラーが出た場合:
- エラーメッセージを `.claude/build-errors.txt` に保存
- エラーの原因を分析し、修正案を提示
