#pragma once

#include <atomic>
#include <cmath>

/// Lock-free Peak/RMS level meter for real-time audio debugging.
/// Audio thread pushes samples, GUI thread reads and resets.
class DebugMeter {
public:
    DebugMeter() = default;
    ~DebugMeter() = default;

    void pushSample(float sample)
    {
        float abs = std::fabs(sample);
        float currentPeak = peak_.load(std::memory_order_relaxed);
        if (abs > currentPeak)
            peak_.store(abs, std::memory_order_relaxed);

        float currentSum = rmsSum_.load(std::memory_order_relaxed);
        rmsSum_.store(currentSum + abs * abs,
                      std::memory_order_relaxed);

        int currentCount = rmsCount_.load(std::memory_order_relaxed);
        rmsCount_.store(currentCount + 1,
                        std::memory_order_relaxed);
    }

    void pushBlock(const float* data, int numSamples)
    {
        float blockPeak = 0.0f;
        float blockSum = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float abs = std::fabs(data[i]);
            if (abs > blockPeak) blockPeak = abs;
            blockSum += abs * abs;
        }

        float currentPeak = peak_.load(std::memory_order_relaxed);
        if (blockPeak > currentPeak)
            peak_.store(blockPeak, std::memory_order_relaxed);

        float currentSum = rmsSum_.load(std::memory_order_relaxed);
        rmsSum_.store(currentSum + blockSum,
                      std::memory_order_relaxed);

        int currentCount = rmsCount_.load(std::memory_order_relaxed);
        rmsCount_.store(currentCount + numSamples,
                        std::memory_order_relaxed);
    }

    struct Levels {
        float peakDb = -100.0f;
        float rmsDb  = -100.0f;
    };

    Levels getAndReset()
    {
        Levels result;

        float p = peak_.exchange(0.0f, std::memory_order_relaxed);
        float sum = rmsSum_.exchange(0.0f, std::memory_order_relaxed);
        int count = rmsCount_.exchange(0, std::memory_order_relaxed);

        result.peakDb = (p > 1e-10f)
            ? 20.0f * std::log10(p) : -100.0f;

        if (count > 0 && sum > 1e-20f)
        {
            float rmsLin = std::sqrt(sum / static_cast<float>(count));
            result.rmsDb = 20.0f * std::log10(rmsLin);
        }

        return result;
    }

    float peekPeakLinear() const
    {
        return peak_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<float> peak_{0.0f};
    std::atomic<float> rmsSum_{0.0f};
    std::atomic<int> rmsCount_{0};
};
