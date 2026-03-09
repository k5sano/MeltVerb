# MeltVerb 設計書

## アーキテクチャ
```
MeltVerbPlugin : juce::AudioProcessor
├── MeltEngine (統合 DSP コア)
│   ├── InputDiffuser ← SpaceEngine 版 (AP×4, smear LFO)
│   ├── MeltDelay ← ModDelay + ModDelayLine 統合
│   │   ├── Normal / Reverse / Swell 3モード
│   │   ├── Hermite 補間
│   │   └── 三角波+正弦波 LFO ミックス
│   ├── ToneFilter ← SpaceEngine 版 (tilt EQ)
│   └── ReverbTank ← SpaceEngine 版 (Dattorro loop)
├── DebugMeter ×6 (input, delay_out, reverb_in, reverb_out, crossfeed, output)
├── LevelMeter (Input/Output RMS for GUI bars)
└── PresetManager
```

## 48kHz ネイティブ化 (MeltDelay)
- kBufferSize: sr * 2.1 の nextPow2
- kMaxDelaySamples: sr * 2.0
- kLfoFreq: 0.7 / sr
- kReverseGrain: sr * 0.05
- envelope follower: attack 1/(sr*0.01), release 1/(sr*0.10)

## Soft-clip
std::tanh(std::clamp(x, -4.0f, 4.0f))
