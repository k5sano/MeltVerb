# MeltVerb 仕様書

## 概要
EarthQuaker Devices Avalanche Run にインスパイアされた空間系 VST3 プラグイン。
MeltCosmos の SpaceEngine + AvalancheEngine を統合。

## シグナルチェーン
mono = (L+R)*0.5 → InputDiffuser(AP×4) → CrossFeed(reverbTail*cf + delayOut*fb)
→ ToneFilter → MeltDelay(write: diffused+fbFiltered, read: mode依存)
→ delayMix blend → ReverbTank → reverbMix blend → clamp±4 + tanh → output

## パラメータ (12個)
- delay_time (0-1, default 0.3, 二乗→0-2000ms)
- delay_feedback (0-0.95, default 0.4)
- delay_tone (0-1, default 0.5)
- delay_mix (0-1, default 0.5)
- delay_mode (int 0-2, default 0: Normal/Reverse/Swell)
- reverb_decay (0-1, default 0.5)
- reverb_damping (0-1, default 0.7)
- reverb_mix (0-1, default 0.5)
- diffusion (0-1, default 0.625)
- cross_feed (0-1, default 0.3)
- mod_speed (0-1, default 0.5)
- mod_depth (0-1, default 0.5)

## DSP モジュール
- InputDiffuser: AP×4 + smear LFO (SpaceEngine版)
- MeltDelay: ModDelay+ModDelayLine統合、48kHzネイティブ、3モード、Hermite補間
- ToneFilter: Tilt EQ (SpaceEngine版)
- ReverbTank: Dattorro loop (SpaceEngine版)
- DebugMeter: 6点 lock-free meter (AvalancheEngine版)

## GUI
720×400, 11ノブ + Mode ComboBox + In/Out メーター + プリセット
