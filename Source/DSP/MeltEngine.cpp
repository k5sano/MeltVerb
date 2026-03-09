#include "MeltEngine.h"
#include <cmath>
#include <algorithm>

MeltEngine::MeltEngine()
{
    prepare(48000.0);
}

void MeltEngine::prepare(double sampleRate)
{
    diffuser_.prepare(sampleRate);
    delay_.prepare(sampleRate);
    tone_.prepare(sampleRate);
    tank_.prepare(sampleRate);

    delayOutPrev_ = 0.0f;
    reverbTailPrev_ = 0.0f;
}

void MeltEngine::clear()
{
    diffuser_.clear();
    delay_.clear();
    tone_.clear();
    tank_.clear();
    delayOutPrev_ = 0.0f;
    reverbTailPrev_ = 0.0f;
}

void MeltEngine::setDiffusion(float k)
{
    diffuser_.setDiffusion(k);
    tank_.setDiffusion(k);
}

void MeltEngine::setModSpeed(float s)
{
    diffuser_.setModSpeed(s);
    delay_.setModSpeed(s);
    tank_.setModSpeed(s);
}

void MeltEngine::process(float* inOutL, float* inOutR,
                          int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = (inOutL[i] + inOutR[i]) * 0.5f;
        float dry = mono;

        // [METER] input
        meterInput.pushSample(mono);

        // 1. Input Diffusion AP x4
        float diffused = diffuser_.process(mono);

        // 2. Cross-feed: reverb tail -> delay feedback
        float fbSignal = delayOutPrev_ * feedback_
                        + reverbTailPrev_ * crossFeed_;

        // [METER] crossfeed
        meterCrossFeed.pushSample(reverbTailPrev_ * crossFeed_);

        // 3. Tone filter on feedback
        float fbFiltered = tone_.process(fbSignal);

        // 4. Write to delay (diffused + filtered feedback)
        delay_.write(diffused + fbFiltered);

        // 5. Provide current input for swell envelope
        delay_.setCurrentInput(mono);

        // 6. Read from delay (mode-dependent)
        float delayOut = delay_.read();

        // [METER] delay_out
        meterDelayOut.pushSample(delayOut);

        // 7. Delay mix -> Tank input
        float tankInput = diffused * (1.0f - delayMix_)
                         + delayOut * delayMix_;

        // [METER] reverb_in
        meterReverbIn.pushSample(tankInput);

        // 8. Reverb Tank (Dattorro loop)
        TankOutput tankOut = tank_.process(tankInput);

        // [METER] reverb_out
        meterReverbOut.pushSample(
            (tankOut.wetL + tankOut.wetR) * 0.5f);

        // 9. Final output: dry/wet via reverb_mix
        float outL = dry * (1.0f - reverbMix_)
                    + tankOut.wetL * reverbMix_;
        float outR = dry * (1.0f - reverbMix_)
                    + tankOut.wetR * reverbMix_;

        // Soft-clip: clamp +-4.0 then tanh
        outL = std::tanh(std::clamp(outL, -4.0f, 4.0f));
        outR = std::tanh(std::clamp(outR, -4.0f, 4.0f));

        // [METER] output
        meterOutput.pushSample((outL + outR) * 0.5f);

        inOutL[i] = outL;
        inOutR[i] = outR;

        // 10. Carry over for next sample
        delayOutPrev_   = delayOut;
        reverbTailPrev_ = tankOut.tail;
    }
}
