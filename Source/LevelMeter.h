#pragma once

#include <atomic>
#include <cmath>
#include <algorithm>

/// Thread-safe RMS level meter for GUI bars.
/// Audio thread writes; GUI thread reads (atomic).
class LevelMeterSimple {
public:
    void reset()
    {
        inLevel_.store(0.0f, std::memory_order_relaxed);
        outLevel_.store(0.0f, std::memory_order_relaxed);
    }

    void measureInput(const float* L, const float* R, int n)
    {
        inLevel_.store(calcRms(L, R, n),
                       std::memory_order_relaxed);
    }

    void measureOutput(const float* L, const float* R, int n)
    {
        outLevel_.store(calcRms(L, R, n),
                        std::memory_order_relaxed);
    }

    float getInputLevel() const
    {
        return inLevel_.load(std::memory_order_relaxed);
    }

    float getOutputLevel() const
    {
        return outLevel_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<float> inLevel_{0.0f};
    std::atomic<float> outLevel_{0.0f};

    static float calcRms(const float* L, const float* R, int n)
    {
        if (n <= 0) return 0.0f;
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
        {
            float m = (L[i] + R[i]) * 0.5f;
            sum += static_cast<double>(m) * m;
        }
        float rms = static_cast<float>(std::sqrt(sum / n));
        if (rms < 1e-6f) return 0.0f;
        float db = 20.0f * std::log10(rms);
        return std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
    }
};
