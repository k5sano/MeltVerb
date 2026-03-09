#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

class PresetManager {
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    void savePreset(const juce::String& name);
    void loadPreset(const juce::String& name);
    void deletePreset(const juce::String& name);

    juce::StringArray getPresetList() const;
    juce::String getCurrentPresetName() const { return currentPreset_; }

private:
    juce::AudioProcessorValueTreeState& apvts_;
    juce::String currentPreset_;

    juce::File getPresetDirectory() const;
    juce::File getPresetFile(const juce::String& name) const;
};
