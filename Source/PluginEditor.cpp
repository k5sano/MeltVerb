#include "PluginEditor.h"

static const juce::Colour kDelayColour  {0xFFDD7744};
static const juce::Colour kReverbColour {0xFF5599DD};
static const juce::Colour kSharedColour {0xFF44BB66};
static const juce::Colour kBgColour     {0xFF1A1A2E};
static const juce::Colour kMeterIn      {0xFF66DDAA};
static const juce::Colour kMeterOut     {0xFFDD6666};

MeltVerbEditor::MeltVerbEditor(MeltVerbPlugin& p)
    : AudioProcessorEditor(p), proc_(p)
{
    // Delay
    setupKnob(timeKnob,    timeLabel,    "Time",    kDelayColour);
    setupKnob(repeatsKnob, repeatsLabel, "Repeats", kDelayColour);
    setupKnob(toneKnob,    toneLabel,    "Tone",    kDelayColour);
    setupKnob(dMixKnob,    dMixLabel,    "D.Mix",   kDelayColour);

    // Mode ComboBox
    addAndMakeVisible(modeBox);
    modeBox.addItem("Normal",  1);
    modeBox.addItem("Reverse", 2);
    modeBox.addItem("Swell",   3);
    modeAtt = std::make_unique<CBAtt>(p.apvts, "delay_mode", modeBox);

    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId,
                        kDelayColour.brighter(0.4f));
    modeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));

    // Reverb
    setupKnob(decayKnob,   decayLabel,   "Decay",   kReverbColour);
    setupKnob(dampingKnob, dampingLabel, "Damping", kReverbColour);
    setupKnob(rMixKnob,    rMixLabel,    "R.Mix",   kReverbColour);

    // Shared
    setupKnob(diffKnob,      diffLabel,      "Diffusion",  kSharedColour);
    setupKnob(crossFeedKnob, crossFeedLabel, "XFeed",      kSharedColour);
    setupKnob(modSpeedKnob,  modSpeedLabel,  "Mod Spd",    kSharedColour);
    setupKnob(modDepthKnob,  modDepthLabel,  "Mod Dep",    kSharedColour);

    // Attachments
    auto& a = p.apvts;
    timeAtt      = std::make_unique<Att>(a, "delay_time",      timeKnob);
    repeatsAtt   = std::make_unique<Att>(a, "delay_feedback",   repeatsKnob);
    toneAtt      = std::make_unique<Att>(a, "delay_tone",       toneKnob);
    dMixAtt      = std::make_unique<Att>(a, "delay_mix",        dMixKnob);
    decayAtt     = std::make_unique<Att>(a, "reverb_decay",     decayKnob);
    dampingAtt   = std::make_unique<Att>(a, "reverb_damping",   dampingKnob);
    rMixAtt      = std::make_unique<Att>(a, "reverb_mix",       rMixKnob);
    diffAtt      = std::make_unique<Att>(a, "diffusion",        diffKnob);
    crossFeedAtt = std::make_unique<Att>(a, "cross_feed",       crossFeedKnob);
    modSpeedAtt  = std::make_unique<Att>(a, "mod_speed",        modSpeedKnob);
    modDepthAtt  = std::make_unique<Att>(a, "mod_depth",        modDepthKnob);

    // Preset controls
    addAndMakeVisible(presetBox);
    presetBox.setTextWhenNothingSelected("-- Presets --");
    presetBox.onChange = [this] {
        auto name = presetBox.getText();
        if (name.isNotEmpty() && name != "-- Presets --")
            proc_.presetManager.loadPreset(name);
    };

    addAndMakeVisible(saveBtn);
    saveBtn.onClick = [this] {
        auto name = proc_.presetManager.getCurrentPresetName();
        auto dlg = std::make_shared<juce::AlertWindow>(
            "Save Preset", "Enter preset name:",
            juce::MessageBoxIconType::NoIcon, this);
        dlg->addTextEditor("name", name, "Name:");
        dlg->addButton("Save", 1);
        dlg->addButton("Cancel", 0);
        dlg->enterModalState(true,
            juce::ModalCallbackFunction::create(
                [this, dlg](int result) {
                    if (result == 1) {
                        auto n = dlg->getTextEditorContents("name").trim();
                        if (n.isNotEmpty()) {
                            proc_.presetManager.savePreset(n);
                            refreshPresetList();
                        }
                    }
                }));
    };

    addAndMakeVisible(delBtn);
    delBtn.onClick = [this] {
        auto name = presetBox.getText();
        if (name.isNotEmpty()) {
            proc_.presetManager.deletePreset(name);
            refreshPresetList();
        }
    };

    // Image
    addAndMakeVisible(imgBtn);
    imgBtn.onClick = [this] { loadBgImage(); };

    addAndMakeVisible(opacityKnob);
    opacityKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    opacityKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    opacityKnob.setRange(0.0, 1.0, 0.01);
    opacityKnob.setValue(0.3);
    opacityKnob.setColour(juce::Slider::rotarySliderFillColourId,
                           juce::Colours::white.withAlpha(0.5f));
    opacityKnob.onValueChange = [this] {
        imgOpacity_ = static_cast<float>(opacityKnob.getValue());
        repaint();
    };

    addAndMakeVisible(opacityLabel);
    opacityLabel.setText("Op", juce::dontSendNotification);
    opacityLabel.setJustificationType(juce::Justification::centred);
    opacityLabel.setColour(juce::Label::textColourId,
                           juce::Colours::white.withAlpha(0.5f));
    opacityLabel.setFont(juce::FontOptions(11.0f));

    refreshPresetList();
    restoreImgPath();

    setSize(720, 400);
    startTimerHz(30);
}

void MeltVerbEditor::timerCallback()
{
    inLevel_  = proc_.levelMeter.getInputLevel();
    outLevel_ = proc_.levelMeter.getOutputLevel();
    repaint(getLocalBounds().removeFromRight(40));
}

void MeltVerbEditor::setupKnob(juce::Slider& knob, juce::Label& label,
                                const juce::String& text, juce::Colour col)
{
    addAndMakeVisible(knob);
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    knob.setColour(juce::Slider::rotarySliderFillColourId, col);
    knob.setColour(juce::Slider::thumbColourId, col.brighter(0.3f));
    knob.setColour(juce::Slider::textBoxTextColourId,
                   juce::Colours::white);
    knob.setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colours::transparentBlack);

    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, col.brighter(0.4f));
    label.setFont(juce::FontOptions(13.0f, juce::Font::bold));
}

void MeltVerbEditor::refreshPresetList()
{
    presetBox.clear(juce::dontSendNotification);
    auto list = proc_.presetManager.getPresetList();
    for (int i = 0; i < list.size(); ++i)
        presetBox.addItem(list[i], i + 1);

    auto cur = proc_.presetManager.getCurrentPresetName();
    if (cur.isNotEmpty()) {
        int idx = list.indexOf(cur);
        if (idx >= 0)
            presetBox.setSelectedId(idx + 1, juce::dontSendNotification);
    }
}

void MeltVerbEditor::loadBgImage()
{
    auto ch = std::make_shared<juce::FileChooser>(
        "Select Background Image",
        juce::File::getSpecialLocation(juce::File::userPicturesDirectory),
        "*.png;*.jpg;*.jpeg;*.gif;*.bmp");

    ch->launchAsync(juce::FileBrowserComponent::openMode
                    | juce::FileBrowserComponent::canSelectFiles,
        [this, ch](const juce::FileChooser&) {
            auto file = ch->getResult();
            if (file.existsAsFile()) {
                bgImage = juce::ImageFileFormat::loadFrom(file);
                saveImgPath(file);
                repaint();
            }
        });
}

juce::File MeltVerbEditor::getImgSettingsFile() const
{
    return juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory)
        .getChildFile("MeltVerb").getChildFile("bg_image.txt");
}

void MeltVerbEditor::saveImgPath(const juce::File& f)
{
    auto sf = getImgSettingsFile();
    sf.getParentDirectory().createDirectory();
    sf.replaceWithText(f.getFullPathName());
}

void MeltVerbEditor::restoreImgPath()
{
    auto sf = getImgSettingsFile();
    if (sf.existsAsFile()) {
        juce::File f(sf.loadFileAsString().trim());
        if (f.existsAsFile())
            bgImage = juce::ImageFileFormat::loadFrom(f);
    }
}

void MeltVerbEditor::drawMeter(juce::Graphics& g,
                                juce::Rectangle<int> area,
                                float level, juce::Colour col)
{
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(area.toFloat(), 3.0f);

    int fillH = static_cast<int>(
        static_cast<float>(area.getHeight()) * level);
    auto filled = area.removeFromBottom(fillH);
    g.setColour(col.withAlpha(0.8f));
    g.fillRoundedRectangle(filled.toFloat(), 3.0f);
}

void MeltVerbEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Background image
    if (bgImage.isValid() && imgOpacity_ > 0.001f) {
        auto area = getLocalBounds().toFloat();
        float iw = static_cast<float>(bgImage.getWidth());
        float ih = static_cast<float>(bgImage.getHeight());
        float sc = std::max(area.getWidth() / iw,
                            area.getHeight() / ih);
        float dw = iw * sc, dh = ih * sc;
        float x = (area.getWidth() - dw) * 0.5f;
        float y = (area.getHeight() - dh) * 0.5f;
        g.setOpacity(imgOpacity_);
        g.drawImage(bgImage, x, y, dw, dh,
                    0, 0, bgImage.getWidth(), bgImage.getHeight());
        g.setOpacity(1.0f);
    }

    // Section backgrounds
    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(40);
    bounds.removeFromRight(40);
    bounds.removeFromTop(4);

    int totalCols = 12; // 5 delay + 3 reverb + 4 shared
    int colW = bounds.getWidth() / totalCols;

    // Delay section (5 cols: 4 knobs + mode)
    auto delArea = bounds.removeFromLeft(colW * 5);
    g.setColour(kDelayColour.withAlpha(0.08f));
    g.fillRoundedRectangle(delArea.toFloat().reduced(2), 8.0f);
    g.setColour(kDelayColour.withAlpha(0.3f));
    g.drawRoundedRectangle(delArea.toFloat().reduced(2), 8.0f, 1.0f);

    // Reverb section (3 cols)
    auto revArea = bounds.removeFromLeft(colW * 3);
    g.setColour(kReverbColour.withAlpha(0.08f));
    g.fillRoundedRectangle(revArea.toFloat().reduced(2), 8.0f);
    g.setColour(kReverbColour.withAlpha(0.3f));
    g.drawRoundedRectangle(revArea.toFloat().reduced(2), 8.0f, 1.0f);

    // Shared section (4 cols)
    auto shArea = bounds;
    g.setColour(kSharedColour.withAlpha(0.08f));
    g.fillRoundedRectangle(shArea.toFloat().reduced(2), 8.0f);
    g.setColour(kSharedColour.withAlpha(0.3f));
    g.drawRoundedRectangle(shArea.toFloat().reduced(2), 8.0f, 1.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("MeltVerb",
               getLocalBounds().reduced(10, 6).removeFromTop(20),
               juce::Justification::right);

    // Level meters
    auto meterStrip = getLocalBounds().reduced(8)
                          .removeFromRight(32);
    meterStrip.removeFromTop(44);

    auto inArea  = meterStrip.removeFromLeft(12);
    meterStrip.removeFromLeft(4);
    auto outArea = meterStrip.removeFromLeft(12);

    drawMeter(g, inArea,  inLevel_,  kMeterIn);
    drawMeter(g, outArea, outLevel_, kMeterOut);

    g.setFont(juce::FontOptions(9.0f));
    g.setColour(kMeterIn);
    g.drawText("In",  inArea.withY(inArea.getBottom() + 2).withHeight(12),
               juce::Justification::centred);
    g.setColour(kMeterOut);
    g.drawText("Out", outArea.withY(outArea.getBottom() + 2).withHeight(12),
               juce::Justification::centred);
}

void MeltVerbEditor::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Preset bar
    auto presetArea = bounds.removeFromTop(36);
    presetBox.setBounds(presetArea.removeFromLeft(220));
    presetArea.removeFromLeft(6);
    saveBtn.setBounds(presetArea.removeFromLeft(50));
    presetArea.removeFromLeft(4);
    delBtn.setBounds(presetArea.removeFromLeft(42));
    presetArea.removeFromLeft(8);
    imgBtn.setBounds(presetArea.removeFromLeft(40));
    presetArea.removeFromLeft(4);
    opacityLabel.setBounds(presetArea.removeFromLeft(20).withHeight(16));
    opacityKnob.setBounds(presetArea.removeFromLeft(34));

    bounds.removeFromTop(4);
    bounds.removeFromRight(40);

    int totalCols = 12;
    int colW = bounds.getWidth() / totalCols;
    int labelH = 14;

    auto placeKnob = [&](juce::Slider& knob, juce::Label& label,
                          juce::Rectangle<int> area) {
        label.setBounds(area.removeFromTop(labelH));
        knob.setBounds(area.reduced(2));
    };

    // Delay (4 knobs + mode combo)
    placeKnob(timeKnob,    timeLabel,    bounds.removeFromLeft(colW));
    placeKnob(repeatsKnob, repeatsLabel, bounds.removeFromLeft(colW));
    placeKnob(toneKnob,    toneLabel,    bounds.removeFromLeft(colW));
    placeKnob(dMixKnob,    dMixLabel,    bounds.removeFromLeft(colW));

    // Mode combo in its own column
    auto modeArea = bounds.removeFromLeft(colW);
    modeLabel.setBounds(modeArea.removeFromTop(labelH));
    modeBox.setBounds(modeArea.reduced(4).removeFromTop(28));

    // Reverb (3)
    placeKnob(decayKnob,   decayLabel,   bounds.removeFromLeft(colW));
    placeKnob(dampingKnob, dampingLabel, bounds.removeFromLeft(colW));
    placeKnob(rMixKnob,    rMixLabel,    bounds.removeFromLeft(colW));

    // Shared (4)
    placeKnob(diffKnob,      diffLabel,      bounds.removeFromLeft(colW));
    placeKnob(crossFeedKnob, crossFeedLabel, bounds.removeFromLeft(colW));
    placeKnob(modSpeedKnob,  modSpeedLabel,  bounds.removeFromLeft(colW));
    placeKnob(modDepthKnob,  modDepthLabel,  bounds);
}
