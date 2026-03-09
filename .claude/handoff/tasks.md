# MeltVerb タスク一覧

## Phase 1: スキャフォールド
- [x] CLAUDE.md
- [x] .claude/ 設定ファイル
- [x] .gitignore
- [x] CMakeLists.txt
- [x] Source/Parameters.h
- [ ] /build-check 確認

## Phase 2: DSP コア
- [ ] InputDiffuser.h
- [ ] ToneFilter.h
- [ ] ReverbTank.h + .cpp
- [ ] MeltDelay.h
- [ ] DebugMeter.h
- [ ] MeltEngine.h + .cpp
- [ ] /build-check 確認

## Phase 3: プラグイン層
- [ ] LevelMeter.h
- [ ] PresetManager.h + .cpp
- [ ] PluginProcessor.h + .cpp
- [ ] PluginEditor.h + .cpp
- [ ] /build-check 確認

## Phase 4: テスト＆バリデーション
- [ ] MeltEngineTests.cpp
- [ ] scripts/run-pluginval.sh
- [ ] /build-check 全テスト通過

## Phase 5: 最終確認
- [ ] 全ビルド成功
- [ ] 全テスト PASS
- [ ] commit & push
