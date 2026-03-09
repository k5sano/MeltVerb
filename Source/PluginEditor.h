#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MeltVerbEditor : public juce::AudioProcessorEditor,
                       private juce::Timer {
public:
    explicit MeltVerbEditor(MeltVerbPlugin&);
    ~MeltVerbEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MeltVerbPlugin& proc_;

    // Delay knobs
    juce::Slider timeKnob, repeatsKnob, toneKnob, dMixKnob;
    juce::Label  timeLabel, repeatsLabel, toneLabel, dMixLabel;

    // Mode combo
    juce::ComboBox modeBox;
    juce::Label modeLabel;
    using CBAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<CBAtt> modeAtt;

    // Reverb knobs
    juce::Slider decayKnob, dampingKnob, rMixKnob;
    juce::Label  decayLabel, dampingLabel, rMixLabel;

    // Shared knobs
    juce::Slider diffKnob, crossFeedKnob, modSpeedKnob, modDepthKnob;
    juce::Label  diffLabel, crossFeedLabel, modSpeedLabel, modDepthLabel;

    // Slider attachments (11)
    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> timeAtt, repeatsAtt, toneAtt, dMixAtt;
    std::unique_ptr<Att> decayAtt, dampingAtt, rMixAtt;
    std::unique_ptr<Att> diffAtt, crossFeedAtt, modSpeedAtt, modDepthAtt;

    // Preset
    juce::ComboBox presetBox;
    juce::TextButton saveBtn{"Save"}, delBtn{"Del"};

    // Background image
    juce::Image bgImage;
    juce::TextButton imgBtn{"Img"};
    juce::Slider opacityKnob;
    juce::Label opacityLabel;
    float imgOpacity_ = 0.3f;

    // Level meters
    float inLevel_  = 0.0f;
    float outLevel_ = 0.0f;

    void setupKnob(juce::Slider&, juce::Label&,
                   const juce::String&, juce::Colour);
    void refreshPresetList();
    void loadBgImage();
    juce::File getImgSettingsFile() const;
    void saveImgPath(const juce::File&);
    void restoreImgPath();
    void drawMeter(juce::Graphics&, juce::Rectangle<int>,
                   float level, juce::Colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeltVerbEditor)
};
