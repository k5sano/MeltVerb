#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Unified modulated delay: ModDelay (3 modes) + ModDelayLine (Hermite).
/// 48kHz native. Normal/Reverse/Swell modes.
/// Triangle + sine LFO mix controlled by mod_depth.
class MeltDelay {
public:
    void prepare(double sampleRate)
    {
        sr_ = std::max(sampleRate, 8000.0);
        maxDelaySamples_ = static_cast<float>(sr_ * 4.0);

        int maxSamples = static_cast<int>(sr_ * 4.1);
        bufSize_ = nextPow2(maxSamples + 4);
        bufMask_ = bufSize_ - 1;
        buf_ = std::make_unique<float[]>(bufSize_);

        reverseGrain_ = static_cast<int>(sr_ * 0.05);
        delaySamples_ = static_cast<float>(0.2 * sr_);  // Step8 Task 8: デフォルト 200ms
        // DelaySmoothing: 約50msでターゲットに到達
        delaySmoothing_     = 1.0f - std::exp(
            -1.0f / static_cast<float>(sr_ * 0.05));
        delaySamplesTarget_ = delaySamples_;
        delaySamplesSmooth_ = delaySamples_;

        envAttack_  = 1.0f - std::exp(-1.0f / static_cast<float>(sr_ * 0.01));
        envRelease_ = 1.0f - std::exp(-1.0f / static_cast<float>(sr_ * 0.10));

        // Euclidean深化 Task B2: スムーズゲイン係数（SR依存）
        euclidGainAttack_  = 1.0f - std::exp(
            -1.0f / static_cast<float>(sr_ * 0.005));
        euclidGainRelease_ = 1.0f - std::exp(
            -1.0f / static_cast<float>(sr_ * 0.020));
        euclidRotationPhase_  = 0.0f;
        euclidRotationOffset_ = 0;
        euclidRotationFreq_   = 0.0f;

        triFreq_ = 0.0f;
        sinFreq_ = 0.0f;
        setModSpeed(0.5f);

        clear();
    }

    void clear()
    {
        if (buf_)
            std::memset(buf_.get(), 0,
                sizeof(float) * static_cast<size_t>(bufSize_));
        wp_ = 0;
        triPhase_ = 0.0f;
        sinPhase_ = 0.0f;
        envelopeFollower_ = 0.0f;
        reverseReadPos_ = 0;
        reverseBlockLen_ = 0;
        reverseCount_ = 0;
        // Euclidean delay Task B: clear時にリセット
        euclidStep_        = 0;
        euclidSampleCount_ = 0;
        euclidGain_        = 0.0f;
        // Euclidean深化 Task B4: ローテーション+スムーズゲインもリセット
        euclidRotationPhase_  = 0.0f;
        euclidRotationOffset_ = 0;
        euclidGainSmooth_     = 0.0f;
        // DelaySmoothing: リセット
        delaySamplesTarget_ = delaySamples_;
        delaySamplesSmooth_ = delaySamples_;
    }

    // DelaySmoothing: 目標値のみ更新、実際の適用は read() 内で
    void setTimeMs(float ms)
    {
        float newTarget = ms * 0.001f * static_cast<float>(sr_);
        delaySamplesTarget_ = std::clamp(newTarget, 1.0f,
            maxDelaySamples_);
        // 大きなジャンプ時はスムーズ値も即座に寄せる（起動直後・プリセット切替時）
        float diff = delaySamplesTarget_ - delaySamplesSmooth_;
        if (std::fabs(diff) > maxDelaySamples_ * 0.25f)
            delaySamplesSmooth_ = delaySamplesTarget_;
        // Euclidean ステップ長は即時更新 OK
        euclidStepLen_ = std::max(1,
            static_cast<int>(delaySamplesTarget_));
    }

    // Euclidean delay Task B: depth変更時にパターン再生成
    void setModDepth(float depth)
    {
        modDepth_ = depth;
        if (mode_ == 3)
        {
            int pulses = std::max(1,
                static_cast<int>(std::round(depth * (kEuclidSteps - 1))) + 1);
            buildEuclidPattern(pulses);
        }
    }

    void setModSpeed(float speed)
    {
        float inv = 1.0f / static_cast<float>(sr_);
        // Triangle LFO: 0.1–5 Hz
        triFreq_ = (0.1f + speed * 4.9f) * inv;
        // Sine LFO: 0.7 Hz base, scaled by speed
        float s = 0.2f + speed * 1.6f;
        sinFreq_ = 0.7f * inv * s;

        // Euclidean深化 Task B3: ローテーション速度（speed=0で停止, =1で約4秒/周）
        if (speed > 0.0f)
        {
            float periodSec = 4.0f + (1.0f - speed) * 60.0f;
            euclidRotationFreq_ = 1.0f
                / (periodSec * static_cast<float>(sr_));
        }
        else
        {
            euclidRotationFreq_ = 0.0f;
        }
    }

    // Euclidean delay Task B: モード切替時に状態初期化
    void setMode(int mode)
    {
        mode_ = mode;
        if (mode_ == 3)
        {
            euclidStep_        = 0;
            euclidSampleCount_ = 0;
            // DelaySmoothing: ターゲット値を使う
            euclidStepLen_     = std::max(1,
                static_cast<int>(delaySamplesTarget_));
            int pulses = std::max(1,
                static_cast<int>(
                    std::round(modDepth_ * (kEuclidSteps - 1))) + 1);
            buildEuclidPattern(pulses);
            euclidGain_ = euclidPattern_[0] ? 1.0f : 0.0f;
            euclidGainSmooth_ = euclidGain_;        // Euclidean深化 Task B6
            euclidRotationPhase_  = 0.0f;           // Euclidean深化 Task B6
            euclidRotationOffset_ = 0;              // Euclidean深化 Task B6
        }
    }

    void write(float sample)
    {
        buf_[wp_ & bufMask_] = sample;
        ++wp_;
    }

    float read()
    {
        // DelaySmoothing: 毎サンプル目標値に近づける
        delaySamplesSmooth_ += delaySmoothing_
            * (delaySamplesTarget_ - delaySamplesSmooth_);
        delaySamples_ = delaySamplesSmooth_;

        // Advance LFOs
        triPhase_ += triFreq_;
        if (triPhase_ >= 1.0f) triPhase_ -= 1.0f;
        sinPhase_ += sinFreq_;
        if (sinPhase_ >= 1.0f) sinPhase_ -= 1.0f;

        if (mode_ == 1)
            return processReverse();
        // Euclidean delay Task C: mode 3 分岐
        if (mode_ == 3)
            return processEuclidean();

        float triVal = 4.0f * std::abs(triPhase_ - 0.5f) - 1.0f;
        float sinVal = std::sin(2.0f * static_cast<float>(M_PI)
                                * sinPhase_);

        // Mix LFOs: depth=0 → triangle, depth=1 → sine
        float lfoMix = triVal * (1.0f - modDepth_)
                      + sinVal * modDepth_;

        float modSamples = lfoMix * modDepth_ * 0.003f
                           * static_cast<float>(sr_);

        float totalDelay = delaySamples_ + modSamples;
        totalDelay = std::clamp(totalDelay, 1.0f,
            static_cast<float>(bufSize_ - 4));

        float delayed = hermiteRead(totalDelay);

        if (mode_ == 2)
            delayed = processSwell(delayed);

        return delayed;
    }

    /// Store current input for swell envelope tracking
    void setCurrentInput(float input) { currentInput_ = input; }

private:
    std::unique_ptr<float[]> buf_;
    int bufSize_ = 0, bufMask_ = 0, wp_ = 0;
    double sr_ = 48000.0;
    float maxDelaySamples_ = 192000.0f;

    float delaySamples_ = 14400.0f;
    // DelaySmoothing: スムージング用
    float delaySamplesTarget_ = 14400.0f;
    float delaySamplesSmooth_ = 14400.0f;
    float delaySmoothing_     = 0.0f;
    float modDepth_ = 0.5f;
    int mode_ = 0;

    float triPhase_ = 0.0f, triFreq_ = 0.0f;
    float sinPhase_ = 0.0f, sinFreq_ = 0.0f;

    float envelopeFollower_ = 0.0f;
    float envAttack_  = 0.0f;
    float envRelease_ = 0.0f;
    float currentInput_ = 0.0f;

    int reverseReadPos_ = 0;
    int reverseBlockLen_ = 0;
    int reverseCount_ = 0;
    int reverseGrain_ = 2400;

    // Euclidean delay Task A: Euclidean rhythm state
    static constexpr int kEuclidSteps = 8;
    bool  euclidPattern_[kEuclidSteps] = {};
    int   euclidStep_        = 0;
    int   euclidSampleCount_ = 0;
    int   euclidStepLen_     = 0;
    float euclidGain_        = 0.0f;

    // Euclidean深化 Task B1: ローテーション + スムーズゲイン
    float euclidRotationPhase_  = 0.0f;
    float euclidRotationFreq_   = 0.0f;
    int   euclidRotationOffset_ = 0;
    float euclidGainSmooth_     = 0.0f;
    float euclidGainAttack_     = 0.0f;
    float euclidGainRelease_    = 0.0f;

    /// Euclidean delay Task A: Bjorklund (bresenham) でパターン生成
    void buildEuclidPattern(int pulses)
    {
        int n = kEuclidSteps;
        int k = std::clamp(pulses, 1, n);

        for (int i = 0; i < n; ++i)
            euclidPattern_[i] = false;

        int err = 0;
        for (int i = 0; i < n; ++i)
        {
            err += k;
            if (err >= n)
            {
                euclidPattern_[i] = true;
                err -= n;
            }
        }

        // パルス数の保険（bresenham端数補正）
        int count = 0;
        for (int i = 0; i < n; ++i)
            if (euclidPattern_[i]) ++count;
        for (int i = 0; i < n && count < k; ++i)
        {
            if (!euclidPattern_[i])
            {
                euclidPattern_[i] = true;
                ++count;
            }
        }
    }

    static int nextPow2(int n)
    {
        int v = 1;
        while (v < n) v <<= 1;
        return v;
    }

    float hermiteRead(float delaySamp) const
    {
        float rp = static_cast<float>(wp_) - delaySamp;
        int idx = static_cast<int>(std::floor(rp));
        float frac = rp - static_cast<float>(idx);

        float xm1 = buf_[(idx - 1) & bufMask_];
        float x0  = buf_[(idx)     & bufMask_];
        float x1  = buf_[(idx + 1) & bufMask_];
        float x2  = buf_[(idx + 2) & bufMask_];

        float c0 = x0;
        float c1 = 0.5f * (x1 - xm1);
        float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    float processReverse()
    {
        if (reverseCount_ <= 0)
        {
            int grainLen = static_cast<int>(delaySamples_);
            if (grainLen < 32) grainLen = 32;
            reverseBlockLen_ = std::min(grainLen, reverseGrain_);
            reverseReadPos_ = (wp_ - 1 + bufSize_) & bufMask_;
            reverseCount_ = reverseBlockLen_;
        }

        float pos = static_cast<float>(
            reverseBlockLen_ - reverseCount_);
        float norm = pos / static_cast<float>(reverseBlockLen_);
        float window = 0.5f - 0.5f * std::cos(
            2.0f * static_cast<float>(M_PI) * norm);

        int readIdx = (reverseReadPos_ + reverseCount_) & bufMask_;
        float sample = buf_[readIdx] * window;

        --reverseCount_;
        return sample;
    }

    // Euclidean深化 Task B5: ローテーション + スムーズゲイン付き Euclidean 処理
    float processEuclidean()
    {
        // --- ローテーション位相を進める ---
        euclidRotationPhase_ += euclidRotationFreq_;
        if (euclidRotationPhase_ >= 1.0f)
        {
            euclidRotationPhase_ -= 1.0f;
            euclidRotationOffset_ =
                (euclidRotationOffset_ + 1) % kEuclidSteps;
        }

        // --- サンプルカウンタを進めてステップ更新 ---
        ++euclidSampleCount_;
        if (euclidSampleCount_ >= euclidStepLen_)
        {
            euclidSampleCount_ = 0;
            euclidStep_ = (euclidStep_ + 1) % kEuclidSteps;

            // ローテーションオフセットを考慮したパターン読み出し
            int rotatedStep =
                (euclidStep_ + euclidRotationOffset_) % kEuclidSteps;
            euclidGain_ = euclidPattern_[rotatedStep] ? 1.0f : 0.0f;
        }

        // --- ゲインをスムーズに補間 ---
        float targetGain = euclidGain_;
        float coeff = (targetGain > euclidGainSmooth_)
            ? euclidGainAttack_
            : euclidGainRelease_;
        euclidGainSmooth_ += coeff * (targetGain - euclidGainSmooth_);

        // --- 遅延読み出しとゲイン適用 ---
        float delayed = hermiteRead(delaySamples_);
        return delayed * euclidGainSmooth_;
    }

    float processSwell(float delayed)
    {
        float absIn = std::fabs(currentInput_);
        float coeff = (absIn > envelopeFollower_)
            ? envAttack_ : envRelease_;
        envelopeFollower_ += coeff * (absIn - envelopeFollower_);

        float gain = 1.0f - std::min(1.0f, envelopeFollower_ * 8.0f);
        gain = gain * gain;
        return delayed * gain;
    }
};
