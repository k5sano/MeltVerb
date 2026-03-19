#include "ReverbTank.h"

ReverbTank::ReverbTank()
{
    prepare(48000.0);
}

int ReverbTank::nextPow2(int n)
{
    int v = 1;
    while (v < n) v <<= 1;
    return v;
}

int ReverbTank::scale(int ref) const
{
    return std::max(1,
        static_cast<int>(std::round(ref * ratio_)));
}

float ReverbTank::interpRead(int base, int maxLen,
                              float offset) const
{
    offset = std::clamp(offset, 0.0f,
        static_cast<float>(maxLen - 2));
    int ip = static_cast<int>(offset);
    float fr = offset - static_cast<float>(ip);
    float a = bufRead(wp_ + base + ip);
    float b = bufRead(wp_ + base + ip + 1);
    return a + (b - a) * fr;
}

void ReverbTank::prepare(double sampleRate)
{
    sr_ = std::max(sampleRate, 8000.0);
    ratio_ = static_cast<float>(sr_ / 32000.0);

    tapDap1a_ = scale(kRefDap1a);
    tapDap1b_ = scale(kRefDap1b);
    tapDel1_  = scale(kRefDel1);
    tapDap2a_ = scale(kRefDap2a);
    tapDap2b_ = scale(kRefDap2b);
    tapDel2_  = scale(kRefDel2);

    baseDap1a_ = 0;
    baseDap1b_ = baseDap1a_ + tapDap1a_ + 1;
    baseDel1_  = baseDap1b_ + tapDap1b_ + 1;
    baseDap2a_ = baseDel1_  + tapDel1_  + 1;
    baseDap2b_ = baseDap2a_ + tapDap2a_ + 1;
    baseDel2_  = baseDap2b_ + tapDap2b_ + 1;

    int total = baseDel2_ + tapDel2_ + 2;
    bufSize_ = nextPow2(total);
    bufMask_ = bufSize_ - 1;
    buf_ = std::make_unique<float[]>(bufSize_);

    loopModBase_ = 4680.0f * ratio_;
    loopModAmp_  = 100.0f * ratio_;

    float maxMod = loopModBase_ + loopModAmp_;
    if (maxMod >= static_cast<float>(tapDel2_ - 2))
    {
        float avail = static_cast<float>(tapDel2_ - 2);
        loopModBase_ = avail - loopModAmp_;
        if (loopModBase_ < 0.0f)
        {
            loopModBase_ = avail * 0.8f;
            loopModAmp_  = avail * 0.15f;
        }
    }

    float inv = 1.0f / static_cast<float>(sr_);
    lfo_[0].setFreq(0.5f * inv);
    lfo_[1].setFreq(0.3f * inv);

    clear();
}

void ReverbTank::clear()
{
    if (buf_)
        std::memset(buf_.get(), 0,
            sizeof(float) * static_cast<size_t>(bufSize_));
    wp_ = 0;
    lpDecay1_ = 0.0f;
    lpDecay2_ = 0.0f;
    svfA_.reset(); // EMverb融合 Task 2
    svfB_.reset(); // EMverb融合 Task 2
    lfoVal_[0] = 0.0f;
    lfoVal_[1] = 0.0f;
}

void ReverbTank::setModSpeed(float speed)
{
    float s = 0.2f + speed * 1.6f;
    float inv = 1.0f / static_cast<float>(sr_);
    lfo_[0].setFreq(0.5f * inv * s);
    lfo_[1].setFreq(0.3f * inv * s);
}

TankOutput ReverbTank::process(float input)
{
    --wp_;
    if (wp_ < 0) wp_ += bufSize_;

    if ((wp_ & 31) == 0)
    {
        lfoVal_[0] = lfo_[0].next();
        lfoVal_[1] = lfo_[1].next();
    }

    float acc, pr;
    TankOutput out;

    // --- Side A ---
    acc = input;
    float modOfs = loopModBase_ + loopModAmp_ * lfoVal_[1];
    acc += interpRead(baseDel2_, tapDel2_, modOfs) * reverbTime_;

    // EMverb融合 Task 2: SVF ローパス（Side A）
    {
        float lpHz   = 200.0f * std::pow(100.0f, lp_);
        float lpNorm = std::min(lpHz / static_cast<float>(sr_), 0.49f);
        acc = svfA_.process(acc, lpNorm);
    }

    pr = bufRead(wp_ + baseDap1a_ + tapDap1a_ - 1);
    acc += pr * (-kap_);
    bufWrite(wp_ + baseDap1a_, acc);
    acc *= kap_;
    acc += pr;

    pr = bufRead(wp_ + baseDap1b_ + tapDap1b_ - 1);
    acc += pr * kap_;
    bufWrite(wp_ + baseDap1b_, acc);
    acc *= -kap_;
    acc += pr;

    acc = saturate(acc); // EMverb融合 Task 1: Side A ソフトサチュレーション
    bufWrite(wp_ + baseDel1_, acc);
    out.wetL = acc;

    // --- Side B ---
    acc = input;
    // EMverb融合 Task 3: Side B に変調追加（EMverb互換 0.7倍オフセット）
    {
        float modOfsB = loopModBase_ * 0.7f
                      + loopModAmp_  * 0.7f * lfoVal_[0];
        acc += interpRead(baseDel1_, tapDel1_, modOfsB) * reverbTime_;
    }

    // EMverb融合 Task 2: SVF ローパス（Side B）
    {
        float lpHz   = 200.0f * std::pow(100.0f, lp_);
        float lpNorm = std::min(lpHz / static_cast<float>(sr_), 0.49f);
        acc = svfB_.process(acc, lpNorm);
    }

    pr = bufRead(wp_ + baseDap2a_ + tapDap2a_ - 1);
    acc += pr * kap_;
    bufWrite(wp_ + baseDap2a_, acc);
    acc *= -kap_;
    acc += pr;

    pr = bufRead(wp_ + baseDap2b_ + tapDap2b_ - 1);
    acc += pr * (-kap_);
    bufWrite(wp_ + baseDap2b_, acc);
    acc *= kap_;
    acc += pr;

    acc = saturate(acc); // EMverb融合 Task 1: Side B ソフトサチュレーション
    bufWrite(wp_ + baseDel2_, acc);
    out.wetR = acc;

    float tailA = bufRead(wp_ + baseDel1_ + tapDel1_ - 1);
    float tailB = bufRead(wp_ + baseDel2_ + tapDel2_ - 1);
    out.tail = (tailA + tailB) * 0.5f;

    return out;
}
