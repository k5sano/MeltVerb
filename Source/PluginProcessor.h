#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DSP/MeltEngine.h"
#include "LevelMeter.h"
#include "PresetManager.h"

class MeltVerbPlugin : public juce::AudioProcessor {
public:
    MeltVerbPlugin();
    ~MeltVerbPlugin() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MeltVerb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;
    LevelMeterSimple levelMeter;
    MeltEngine& getEngine() { return engine_; }

private:
    std::atomic<float>* delayTimeParam     = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayToneParam     = nullptr;
    std::atomic<float>* delayMixParam      = nullptr;
    std::atomic<float>* delayModeParam     = nullptr;
    std::atomic<float>* reverbDecayParam   = nullptr;
    std::atomic<float>* reverbDampParam    = nullptr;
    std::atomic<float>* reverbMixParam     = nullptr;
    std::atomic<float>* diffusionParam     = nullptr;
    std::atomic<float>* crossFeedParam     = nullptr;
    std::atomic<float>* modSpeedParam      = nullptr;
    std::atomic<float>* modDepthParam      = nullptr;

    std::atomic<float>* diffuseRangeParam    = nullptr;
    std::atomic<float>* diffuseSendParam     = nullptr;
    std::atomic<float>* diffuseReturnParam   = nullptr;

    std::atomic<float>* bypassDiffuserParam  = nullptr;
    std::atomic<float>* bypassDelayParam     = nullptr;
    std::atomic<float>* bypassToneParam      = nullptr;
    std::atomic<float>* bypassReverbParam    = nullptr;
    std::atomic<float>* bypassCrossFeedParam = nullptr;

    MeltEngine engine_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeltVerbPlugin)
};
