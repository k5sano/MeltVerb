#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct TankOutput {
    float wetL = 0.0f;
    float wetR = 0.0f;
    float tail = 0.0f;
};

class ReverbTank {
public:
    ReverbTank();
    ~ReverbTank() = default;

    void prepare(double sampleRate);
    void clear();

    TankOutput process(float input);

    void setDecay(float d)     { reverbTime_ = d; }
    void setDamping(float lp)  { lp_ = lp; }
    void setDiffusion(float k) { kap_ = k; }
    void setModSpeed(float speed);

private:
    static constexpr int kRefDap1a = 1653;
    static constexpr int kRefDap1b = 2038;
    static constexpr int kRefDel1  = 3411;
    static constexpr int kRefDap2a = 1913;
    static constexpr int kRefDap2b = 1663;
    static constexpr int kRefDel2  = 4782;

    int tapDap1a_ = 0, tapDap1b_ = 0, tapDel1_ = 0;
    int tapDap2a_ = 0, tapDap2b_ = 0, tapDel2_ = 0;

    int baseDap1a_ = 0, baseDap1b_ = 0, baseDel1_ = 0;
    int baseDap2a_ = 0, baseDap2b_ = 0, baseDel2_ = 0;

    std::unique_ptr<float[]> buf_;
    int bufSize_ = 0, bufMask_ = 0, wp_ = 0;

    float reverbTime_ = 0.5f;
    float lp_ = 0.7f;
    float kap_ = 0.625f;

    float lpDecay1_ = 0.0f;
    float lpDecay2_ = 0.0f;

    struct CosOsc {
        float phase = 0.0f, freq = 0.0f;
        float next()
        {
            phase += freq;
            if (phase >= 1.0f) phase -= 1.0f;
            return std::cos(2.0f * static_cast<float>(M_PI)
                            * phase);
        }
        void setFreq(float f) { freq = f; }
    };
    CosOsc lfo_[2];
    float lfoVal_[2] = {};

    double sr_ = 48000.0;
    float ratio_ = 1.5f;
    float loopModBase_ = 0.0f;
    float loopModAmp_  = 0.0f;

    int scale(int ref) const;
    static int nextPow2(int n);
    float bufRead(int i) const { return buf_[i & bufMask_]; }
    void bufWrite(int i, float v) { buf_[i & bufMask_] = v; }
    float interpRead(int base, int maxLen, float offset) const;
};
