#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Tilt EQ for delay feedback path.
/// tone=0.0 -> dark (LP emphasis), 0.5 -> flat, 1.0 -> bright (HP emphasis).
/// Implemented as parallel LP + HP with crossfade.
class ToneFilter {
public:
    void prepare(double sampleRate)
    {
        sr_ = std::max(sampleRate, 8000.0);
        updateCoeffs();
        clear();
    }

    void clear()
    {
        lpState_ = 0.0f;
        hpState_ = 0.0f;
        prev_    = 0.0f;
    }

    void setTone(float tone)
    {
        tone_ = std::clamp(tone, 0.0f, 1.0f);
        updateCoeffs();
    }

    float process(float x)
    {
        lpState_ += lpCoeff_ * (x - lpState_);
        float lp = lpState_;

        hpState_ += hpCoeff_ * (x - hpState_);
        float hp = x - hpState_;

        float t = tone_ * 2.0f;
        float out;
        if (t <= 1.0f)
            out = lp * (1.0f - t) + x * t;
        else
            out = x * (2.0f - t) + hp * (t - 1.0f);

        return out;
    }

private:
    double sr_ = 48000.0;
    float tone_ = 0.5f;
    float lpCoeff_ = 0.0f;
    float hpCoeff_ = 0.0f;
    float lpState_ = 0.0f;
    float hpState_ = 0.0f;
    float prev_ = 0.0f;

    void updateCoeffs()
    {
        float wLP = 2.0f * static_cast<float>(M_PI) * 800.0f
                    / static_cast<float>(sr_);
        lpCoeff_ = std::clamp(wLP / (1.0f + wLP), 0.0f, 1.0f);

        float wHP = 2.0f * static_cast<float>(M_PI) * 2500.0f
                    / static_cast<float>(sr_);
        hpCoeff_ = std::clamp(wHP / (1.0f + wHP), 0.0f, 1.0f);
    }
};
