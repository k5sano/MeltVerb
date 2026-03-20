#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Input Diffusion: 4 cascaded allpass filters
/// Extracted from DattorroReverb (Clouds topology).
/// Includes smear LFO on AP1.
class InputDiffuser {
public:
    void prepare(double sampleRate)
    {
        sr_ = std::max(sampleRate, 8000.0);
        ratio_ = static_cast<float>(sr_ / 32000.0);

        tap_[0] = scale(kRef[0]);
        tap_[1] = scale(kRef[1]);
        tap_[2] = scale(kRef[2]);
        tap_[3] = scale(kRef[3]);

        base_[0] = 0;
        for (int i = 1; i < 4; ++i)
            base_[i] = base_[i - 1] + tap_[i - 1] + 1;

        int total = base_[3] + tap_[3] + 2;
        bufSize_ = nextPow2(total);
        bufMask_ = bufSize_ - 1;
        buf_ = std::make_unique<float[]>(bufSize_);

        smearBase_ = 10.0f * ratio_;
        smearAmp_  = 60.0f * ratio_;
        smearWOfs_ = std::min(scale(100), tap_[0] - 1);

        float inv = 1.0f / static_cast<float>(sr_);
        lfoPhase_ = 0.0f;
        lfoFreq_  = 0.5f * inv;
        lfoVal_   = 0.0f;

        clear();
    }

    void clear()
    {
        if (buf_)
            std::memset(buf_.get(), 0,
                sizeof(float) * static_cast<size_t>(bufSize_));
        wp_ = 0;
        lfoVal_ = 0.0f;
    }

    // Fix A: 4段カスケードallpass + smear干渉を考慮した安全上限
    void setDiffusion(float k)
    {
        kap_ = std::clamp(k, 0.0f, 0.82f);
    }

    void setModSpeed(float speed)
    {
        float s = 0.2f + speed * 1.6f;
        lfoFreq_ = 0.5f / static_cast<float>(sr_) * s;
    }

    float process(float input)
    {
        --wp_;
        if (wp_ < 0) wp_ += bufSize_;

        if ((wp_ & 31) == 0)
        {
            lfoPhase_ += lfoFreq_ * 32.0f;
            if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
            lfoVal_ = std::cos(2.0f * static_cast<float>(M_PI)
                               * lfoPhase_);
        }

        float smOfs = smearBase_ + smearAmp_ * lfoVal_;
        // Fix B: smear値をsaturateしてから書き込む（突発的な大振幅注入を防ぐ）
        float smVal = saturate(interpRead(0, tap_[0], smOfs));
        bufWrite(wp_ + base_[0] + smearWOfs_, smVal);

        float acc = input;

        for (int j = 0; j < 4; ++j)
        {
            float rd = bufRead(wp_ + base_[j] + tap_[j] - 1);
            acc += rd * kap_;
            bufWrite(wp_ + base_[j], acc);
            acc *= -kap_;
            acc += rd;
        }

        return acc;
    }

private:
    static constexpr int kRef[4] = {113, 162, 241, 399};

    int tap_[4] = {};
    int base_[4] = {};

    std::unique_ptr<float[]> buf_;
    int bufSize_ = 0, bufMask_ = 0, wp_ = 0;

    float kap_ = 0.625f;
    double sr_ = 48000.0;
    float ratio_ = 1.5f;

    float smearBase_ = 0.0f, smearAmp_ = 0.0f;
    int smearWOfs_ = 0;
    float lfoPhase_ = 0.0f, lfoFreq_ = 0.0f, lfoVal_ = 0.0f;

    int scale(int ref) const
    {
        return std::max(1,
            static_cast<int>(std::round(ref * ratio_)));
    }

    static int nextPow2(int n)
    {
        int v = 1;
        while (v < n) v <<= 1;
        return v;
    }

    float bufRead(int i) const { return buf_[i & bufMask_]; }
    void bufWrite(int i, float v) { buf_[i & bufMask_] = v; }

    // Fix B: Diffuser smear 用ソフトサチュレーション
    static inline float saturate(float x) noexcept
    {
        constexpr float kDrive = 0.7f;
        return std::tanh(x * kDrive) / kDrive;
    }

    float interpRead(int base, int maxLen, float offset) const
    {
        offset = std::clamp(offset, 0.0f,
            static_cast<float>(maxLen - 2));
        int ip = static_cast<int>(offset);
        float fr = offset - static_cast<float>(ip);
        float a = bufRead(wp_ + base + ip);
        float b = bufRead(wp_ + base + ip + 1);
        return a + (b - a) * fr;
    }
};
