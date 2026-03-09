# /build-fix — ビルドエラーの軽微修正

## 修正ルール

1. `.claude/build-errors.txt` のエラーメッセージを確認
2. 仕様書 (`.claude/handoff/spec.md`) で定義されたコードを尊重
3. 修正は根拠がある場合のみ、最小限で行う
4. 同一箇所で3回修正失敗 → `git checkout` で戻して `.claude/escalation-report.md` に報告して停止

## 修正後

`/build-check` を実行して修正を確認すること。
