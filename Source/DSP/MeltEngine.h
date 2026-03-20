#pragma once

#include "InputDiffuser.h"
#include "MeltDelay.h"
#include "ToneFilter.h"
#include "ReverbTank.h"
#include "DebugMeter.h"

/// Unified DSP engine: InputDiffuser → MeltDelay (3 modes) → ReverbTank
/// with cross-feed from reverb tail back into delay feedback.
/// 48kHz native, 32-bit float.
class MeltEngine {
public:
    MeltEngine();
    ~MeltEngine() = default;

    void prepare(double sampleRate);
    void clear();
    void process(float* inOutL, float* inOutR, int numSamples);

    // Delay params
    void setDelayTime(float ms)       { delay_.setTimeMs(ms); } // Step8 Task 8: ms直値
    void setDelayFeedback(float fb)   { feedback_ = fb; }
    void setDelayTone(float t)        { tone_.setTone(t); }
    void setDelayMix(float m)         { delayMix_ = m; }
    void setDelayMode(int mode)       { delay_.setMode(mode); }

    // Reverb params
    void setReverbDecay(float d)      { tank_.setDecay(d); }
    void setReverbDamping(float lp)   { tank_.setDamping(lp); }
    void setReverbMix(float m)        { reverbMix_ = m; }

    // Shared params
    void setDiffusion(float k);
    void setDiffuseRange(int range)   { diffuseRange_ = range; }
    void setDiffuseSend(float s)      { diffuseSend_ = s; }
    void setDiffuseReturn(float r)    { diffuseReturn_ = r; }
    void setCrossFeed(float cf)       { crossFeed_ = cf; }
    void setModSpeed(float s);
    void setModDepth(float d)         { delay_.setModDepth(d); }

    // Bypass switches
    void setBypassDiffuser(bool b)    { bypassDiffuser_ = b; }
    void setBypassDelay(bool b)       { bypassDelay_ = b; }
    void setBypassTone(bool b)        { bypassTone_ = b; }
    void setBypassReverb(bool b)      { bypassReverb_ = b; }
    void setBypassCrossFeed(bool b)   { bypassCrossFeed_ = b; }

    // 6-point debug meters
    DebugMeter meterInput, meterDelayOut, meterReverbIn;
    DebugMeter meterReverbOut, meterCrossFeed, meterOutput;

private:
    InputDiffuser diffuser_;
    MeltDelay     delay_;
    ToneFilter    tone_;
    ReverbTank    tank_;

    float feedback_   = 0.25f;
    float delayMix_   = 0.5f;
    float reverbMix_  = 0.5f;
    float crossFeed_  = 0.1f;

    float delayOutPrev_    = 0.0f;
    float reverbTailPrev_  = 0.0f;

    int   diffuseRange_    = 1;     // 0=Low, 1=Mid, 2=High
    float diffuseSend_     = 0.5f;
    float diffuseReturn_   = 0.5f;
    float diffusionRaw_    = 0.4f;  // raw knob value before range mapping

    // Step7 Task 7: ディレイフィードバック用ソフトサチュレーション
    static inline float saturateFb(float x) noexcept
    {
        constexpr float kDrive = 0.5f;
        return std::tanh(x * kDrive) / kDrive;
    }

    bool bypassDiffuser_   = false;
    bool bypassDelay_      = false;
    bool bypassTone_       = false;
    bool bypassReverb_     = false;
    bool bypassCrossFeed_  = false;
};
