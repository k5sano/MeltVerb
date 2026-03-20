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
    diffusionRaw_ = k;

    // Remap k based on range selector
    float mapped = k;
    switch (diffuseRange_)
    {
        case 0: mapped = k * 0.14f;                break; // Low:  0–0.14
        case 1: mapped = 0.14f + k * 0.71f;        break; // Mid:  0.14–0.85
        case 2: mapped = 0.85f + k * 0.15f;        break; // High: 0.85–1.0
    }

    diffuser_.setDiffusion(mapped);
    tank_.setDiffusion(mapped);
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
        // Step5 Task 5: DSP処理用モノラル + ドライはステレオ保持（前後感・定位の維持）
        float mono = (inOutL[i] + inOutR[i]) * 0.5f;
        float dryL = inOutL[i];
        float dryR = inOutR[i];

        // [METER] input
        meterInput.pushSample(mono);

        // 1. Input Diffusion with send/return control
        float diffused;
        if (bypassDiffuser_)
        {
            diffused = mono;
        }
        else
        {
            float diffIn = mono * diffuseSend_;
            float diffOut = diffuser_.process(diffIn);
            // Step6 Task 6a: Diffuser出力をソフトクリップ
            constexpr float kDiffThresh = 1.5f;
            diffOut = std::tanh(diffOut / kDiffThresh) * kDiffThresh;
            diffused = mono * (1.0f - diffuseReturn_)
                      + diffOut * diffuseReturn_;
        }

        // 2. Cross-feed: reverb tail -> delay feedback
        float effFb = feedback_;
        float effCf = crossFeed_;
        float totalGain = effFb + effCf;
        if (totalGain > 0.95f)
        {
            float scale = 0.95f / totalGain;
            effFb *= scale;
            effCf *= scale;
        }
        float cfSignal = bypassCrossFeed_
            ? 0.0f : reverbTailPrev_ * effCf;
        float fbSignal = delayOutPrev_ * effFb + cfSignal;
        // Step6 Task 6b: fbSignal をソフトクリップ
        constexpr float kFbThresh = 2.0f;
        fbSignal = std::tanh(fbSignal / kFbThresh) * kFbThresh;
        fbSignal = saturateFb(fbSignal);  // Step7 Task 7: fbパスをまろやかに

        // [METER] crossfeed
        meterCrossFeed.pushSample(cfSignal);

        // 3-4. Delay (tone filter applied to delay output, not feedback)
        float delayOut = 0.0f;
        if (bypassDelay_)
        {
            delayOut = diffused;
        }
        else
        {
            // Fix 1: read()を先に呼ぶことで正しい遅延量を得る
            delay_.setCurrentInput(mono);
            float rawDelay = delay_.read();
            delay_.write(mono + fbSignal);
            delayOut = bypassTone_ ? rawDelay : tone_.process(rawDelay);
        }

        // [METER] delay_out
        meterDelayOut.pushSample(delayOut);

        // 7. Delay mix -> Tank input
        float tankInput = diffused * (1.0f - delayMix_)
                         + delayOut * delayMix_;

        // [METER] reverb_in
        meterReverbIn.pushSample(tankInput);

        // 8. Reverb Tank
        float outL, outR;
        if (bypassReverb_)
        {
            // リバーブをスルー: tankInput（ディレイ含む）をそのまま出力
            outL = dryL * (1.0f - reverbMix_) + tankInput * reverbMix_;
            outR = dryR * (1.0f - reverbMix_) + tankInput * reverbMix_;
            meterReverbOut.pushSample(0.0f);
        }
        else
        {
            TankOutput tankOut = tank_.process(tankInput);
            meterReverbOut.pushSample(
                (tankOut.wetL + tankOut.wetR) * 0.5f);

            outL = dryL * (1.0f - reverbMix_)
                  + tankOut.wetL * reverbMix_;
            outR = dryR * (1.0f - reverbMix_)
                  + tankOut.wetR * reverbMix_;

            reverbTailPrev_ = tankOut.tail;
        }

        // EMverb融合 Task 4: ソフトクリップ（tanh でアナログ的な軟飽和）
        constexpr float kOutThresh = 1.5f;
        outL = std::tanh(outL / kOutThresh) * kOutThresh;
        outR = std::tanh(outR / kOutThresh) * kOutThresh;

        // [METER] output
        meterOutput.pushSample((outL + outR) * 0.5f);

        inOutL[i] = outL;
        inOutR[i] = outR;

        // Carry over for next sample
        delayOutPrev_ = delayOut;
        if (bypassReverb_) reverbTailPrev_ = 0.0f;
    }
}
