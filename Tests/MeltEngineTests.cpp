#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "MeltEngine.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float rms(const float* buf, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
        sum += static_cast<double>(buf[i]) * buf[i];
    return static_cast<float>(std::sqrt(sum / n));
}

TEST_CASE("Silence in -> silence out (zero feedback/decay)", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setDelayFeedback(0.0f);
    eng.setReverbDecay(0.0f);
    eng.setReverbMix(1.0f);
    eng.setDelayMix(0.5f);
    eng.setCrossFeed(0.0f);

    constexpr int N = 4096;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    eng.process(L.data(), R.data(), N);

    REQUIRE(rms(L.data(), N) < 1e-6f);
    REQUIRE(rms(R.data(), N) < 1e-6f);
}

TEST_CASE("Impulse produces reverb tail", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setReverbDecay(0.7f);
    eng.setReverbMix(1.0f);
    eng.setDelayMix(0.0f);
    eng.setCrossFeed(0.0f);
    eng.setDiffusion(0.625f);
    eng.setReverbDamping(0.7f);

    constexpr int N = 16000;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;

    eng.process(L.data(), R.data(), N);
    REQUIRE(rms(L.data() + 2000, N - 2000) > 1e-4f);
}

TEST_CASE("Impulse produces delay echo", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setDelayTime(0.5f);       // 0.25 * 2000 = 500ms
    eng.setDelayFeedback(0.0f);
    eng.setDelayMix(1.0f);
    eng.setReverbMix(1.0f);       // must be >0 to hear delay through tank
    eng.setReverbDecay(0.3f);
    eng.setCrossFeed(0.0f);
    eng.setDiffusion(0.625f);

    constexpr int N = 48000;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;

    eng.process(L.data(), R.data(), N);

    // Energy around 500ms mark (delay echo through reverb)
    float midRms = rms(L.data() + 20000, 8000);
    REQUIRE(midRms > 1e-4f);
}

TEST_CASE("Cross-feed increases density", "[melt]")
{
    auto runWithCF = [](float cf) -> float {
        MeltEngine eng;
        eng.prepare(48000.0);
        eng.setDelayTime(0.3f);
        eng.setDelayFeedback(0.4f);
        eng.setDelayMix(0.5f);
        eng.setReverbDecay(0.6f);
        eng.setReverbMix(0.8f);
        eng.setCrossFeed(cf);
        eng.setDiffusion(0.625f);
        eng.setReverbDamping(0.7f);

        constexpr int N = 24000;
        std::vector<float> L(N, 0.0f), R(N, 0.0f);
        L[0] = 1.0f; R[0] = 1.0f;
        eng.process(L.data(), R.data(), N);
        return rms(L.data() + 12000, 12000);
    };

    float noCF   = runWithCF(0.0f);
    float withCF = runWithCF(0.8f);
    REQUIRE(withCF > noCF);
}

TEST_CASE("Clear resets all state", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setReverbDecay(0.9f);
    eng.setDelayFeedback(0.6f);
    eng.setReverbMix(1.0f);
    eng.setDelayMix(0.5f);
    eng.setCrossFeed(0.5f);

    constexpr int N = 4096;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;
    eng.process(L.data(), R.data(), N);

    eng.clear();
    std::fill(L.begin(), L.end(), 0.0f);
    std::fill(R.begin(), R.end(), 0.0f);
    eng.process(L.data(), R.data(), N);

    REQUIRE(rms(L.data(), N) < 1e-6f);
}

TEST_CASE("Works at 44100Hz", "[melt]")
{
    MeltEngine eng;
    eng.prepare(44100.0);
    eng.setReverbDecay(0.5f);
    eng.setDelayTime(0.3f);
    eng.setDelayMix(0.5f);
    eng.setReverbMix(0.8f);

    constexpr int N = 8000;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;
    eng.process(L.data(), R.data(), N);

    REQUIRE(rms(L.data() + 2000, 4000) > 1e-5f);
}

TEST_CASE("Works at 96000Hz", "[melt]")
{
    MeltEngine eng;
    eng.prepare(96000.0);
    eng.setReverbDecay(0.5f);
    eng.setDelayTime(0.3f);
    eng.setDelayMix(0.5f);
    eng.setReverbMix(0.8f);

    constexpr int N = 16000;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;
    eng.process(L.data(), R.data(), N);

    REQUIRE(rms(L.data() + 4000, 8000) > 1e-5f);
}

TEST_CASE("reverbMix=0, delayMix=0 -> dry signal", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setReverbMix(0.0f);
    eng.setDelayMix(0.0f);
    eng.setCrossFeed(0.0f);

    constexpr int N = 256;
    std::vector<float> L(N), R(N), origL(N);
    for (int i = 0; i < N; ++i) {
        float v = std::sin(2.0f * static_cast<float>(M_PI)
                           * 440.0f * i / 48000.0f);
        L[i] = v; R[i] = v; origL[i] = v;
    }

    eng.process(L.data(), R.data(), N);

    float origRms = rms(origL.data(), N);
    float outRms  = rms(L.data(), N);
    float ratio = outRms / std::max(origRms, 1e-10f);
    REQUIRE(ratio > 0.3f);
    REQUIRE(ratio < 3.0f);
}

TEST_CASE("No blowup with max feedback + cross-feed", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setDelayFeedback(0.95f);
    eng.setCrossFeed(1.0f);
    eng.setReverbDecay(0.99f);
    eng.setReverbMix(1.0f);
    eng.setDelayMix(1.0f);

    constexpr int N = 96000;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;
    eng.process(L.data(), R.data(), N);

    for (int i = 0; i < N; ++i) {
        REQUIRE(std::isfinite(L[i]));
        REQUIRE(std::isfinite(R[i]));
        REQUIRE(std::abs(L[i]) <= 1.51f);
        REQUIRE(std::abs(R[i]) <= 1.51f);
    }
}

TEST_CASE("Reverse mode produces output", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setDelayTime(0.3f);
    eng.setDelayFeedback(0.3f);
    eng.setDelayMix(0.8f);
    eng.setReverbMix(0.3f);
    eng.setDelayMode(1);

    constexpr int N = 4096;
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = 1.0f; R[0] = 1.0f;

    eng.process(L.data(), R.data(), N);

    float maxVal = 0.0f;
    for (int i = 1; i < N; ++i)
        maxVal = std::max(maxVal,
            std::max(std::fabs(L[i]), std::fabs(R[i])));
    REQUIRE(maxVal > 0.001f);
}

TEST_CASE("Swell mode attenuates during loud input", "[melt]")
{
    MeltEngine eng;
    eng.prepare(48000.0);
    eng.setDelayTime(0.05f);
    eng.setDelayFeedback(0.0f);
    eng.setDelayMix(1.0f);
    eng.setReverbMix(0.0f);
    eng.setDelayMode(2);

    constexpr int N = 2048;
    std::vector<float> L(N, 0.5f), R(N, 0.5f);

    eng.process(L.data(), R.data(), N);

    float tailEnergy = 0.0f;
    for (int i = N - 256; i < N; ++i)
        tailEnergy += L[i] * L[i] + R[i] * R[i];

    float avgEnergy = tailEnergy / 256.0f;
    REQUIRE(avgEnergy < 2.0f);
}
