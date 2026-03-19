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
        delaySamples_ = static_cast<float>(0.3 * sr_);

        envAttack_  = 1.0f - std::exp(-1.0f / static_cast<float>(sr_ * 0.01));
        envRelease_ = 1.0f - std::exp(-1.0f / static_cast<float>(sr_ * 0.10));

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
    }

    void setTimeNorm(float norm)
    {
        float ms = norm * norm * 4000.0f;
        delaySamples_ = ms * 0.001f * static_cast<float>(sr_);
        delaySamples_ = std::clamp(delaySamples_, 1.0f,
            maxDelaySamples_);
    }

    void setModDepth(float depth) { modDepth_ = depth; }

    void setModSpeed(float speed)
    {
        float inv = 1.0f / static_cast<float>(sr_);
        // Triangle LFO: 0.1–5 Hz
        triFreq_ = (0.1f + speed * 4.9f) * inv;
        // Sine LFO: 0.7 Hz base, scaled by speed
        float s = 0.2f + speed * 1.6f;
        sinFreq_ = 0.7f * inv * s;
    }

    void setMode(int mode) { mode_ = mode; }

    void write(float sample)
    {
        buf_[wp_ & bufMask_] = sample;
        ++wp_;
    }

    float read()
    {
        // Advance LFOs
        triPhase_ += triFreq_;
        if (triPhase_ >= 1.0f) triPhase_ -= 1.0f;
        sinPhase_ += sinFreq_;
        if (sinPhase_ >= 1.0f) sinPhase_ -= 1.0f;

        if (mode_ == 1)
            return processReverse();

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
