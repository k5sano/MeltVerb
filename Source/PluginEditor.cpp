#include "PluginEditor.h"

static const juce::Colour kDelayColour  {0xFFDD7744};
static const juce::Colour kReverbColour {0xFF5599DD};
static const juce::Colour kSharedColour {0xFF44BB66};
static const juce::Colour kBgColour     {0xFF1A1A2E};
static const juce::Colour kMeterIn      {0xFF66DDAA};
static const juce::Colour kMeterOut     {0xFFDD6666};
static const juce::Colour kDebugColour  {0xFFAAAAFF};
static const juce::Colour kBypassOn     {0xFFFF6644};

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
    modeBox.addItem("Normal",     1);
    modeBox.addItem("Reverse",    2);
    modeBox.addItem("Swell",      3);
    modeBox.addItem("Euclidean",  4);  // Euclidean delay Task D
    modeBox.addItem("Stochastic", 5);  // Stochastic Phase1
    modeBox.addItem("Zen",        6);  // Stochastic Phase1: Zen Mode 予約
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

    // Diffuser control
    static const juce::Colour kDiffuserColour {0xFFCC88DD};
    addAndMakeVisible(diffRangeBox);
    diffRangeBox.addItem("Low",  1);
    diffRangeBox.addItem("Mid",  2);
    diffRangeBox.addItem("High", 3);
    diffRangeAtt = std::make_unique<CBAtt>(a, "diffuse_range", diffRangeBox);

    addAndMakeVisible(diffRangeLabel);
    diffRangeLabel.setText("Range", juce::dontSendNotification);
    diffRangeLabel.setJustificationType(juce::Justification::centred);
    diffRangeLabel.setColour(juce::Label::textColourId,
                             kDiffuserColour.brighter(0.3f));
    diffRangeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    setupKnob(diffSendKnob,   diffSendLabel,   "D.Send", kDiffuserColour);
    setupKnob(diffReturnKnob, diffReturnLabel, "D.Ret",  kDiffuserColour);
    diffSendAtt   = std::make_unique<Att>(a, "diffuse_send",   diffSendKnob);
    diffReturnAtt = std::make_unique<Att>(a, "diffuse_return", diffReturnKnob);

    // Bypass toggle buttons
    setupBypassBtn(bypDiffuserBtn);
    setupBypassBtn(bypDelayBtn);
    setupBypassBtn(bypToneBtn);
    setupBypassBtn(bypReverbBtn);
    setupBypassBtn(bypCrossFeedBtn);
    bypDiffuserAtt  = std::make_unique<BtnAtt>(a, "bypass_diffuser",  bypDiffuserBtn);
    bypDelayAtt     = std::make_unique<BtnAtt>(a, "bypass_delay",     bypDelayBtn);
    bypToneAtt      = std::make_unique<BtnAtt>(a, "bypass_tone",      bypToneBtn);
    bypReverbAtt    = std::make_unique<BtnAtt>(a, "bypass_reverb",    bypReverbBtn);
    bypCrossFeedAtt = std::make_unique<BtnAtt>(a, "bypass_crossfeed", bypCrossFeedBtn);

    // PingPongRandom: ランダムパントグルボタン
    addAndMakeVisible(panRandomBtn);
    panRandomBtn.setColour(juce::ToggleButton::textColourId,
        juce::Colours::white.withAlpha(0.8f));
    panRandomBtn.setColour(juce::ToggleButton::tickColourId,
        juce::Colour(0xFFFFDD44));
    panRandomBtn.onStateChange = [this] {
        proc_.getEngine().setPanRandom(panRandomBtn.getToggleState());
    };

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
                [safeThis = juce::Component::SafePointer(this), dlg](int result) {
                    if (safeThis == nullptr) return;
                    if (result == 1) {
                        auto n = dlg->getTextEditorContents("name").trim();
                        if (n.isNotEmpty()) {
                            safeThis->proc_.presetManager.savePreset(n);
                            safeThis->refreshPresetList();
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

    setSize(820, 520);
    startTimerHz(30);
}

void MeltVerbEditor::timerCallback()
{
    inLevel_  = proc_.levelMeter.getInputLevel();
    outLevel_ = proc_.levelMeter.getOutputLevel();

    // Read 6-point debug meters
    auto& eng = proc_.getEngine();
    DebugMeter* meters[kNumDebugMeters] = {
        &eng.meterInput, &eng.meterDelayOut, &eng.meterReverbIn,
        &eng.meterReverbOut, &eng.meterCrossFeed, &eng.meterOutput
    };
    for (int i = 0; i < kNumDebugMeters; ++i)
    {
        auto lev = meters[i]->getAndReset();
        debugLevels_[i] = lev.peakDb;
    }

    repaint();
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

void MeltVerbEditor::setupBypassBtn(juce::ToggleButton& btn)
{
    addAndMakeVisible(btn);
    btn.setColour(juce::ToggleButton::textColourId,
                  juce::Colours::white.withAlpha(0.8f));
    btn.setColour(juce::ToggleButton::tickColourId, kBypassOn);
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

void MeltVerbEditor::drawDebugMeter(juce::Graphics& g,
                                     juce::Rectangle<int> area,
                                     const juce::String& label,
                                     float peakDb)
{
    // Map dB to 0-1: -60dB=0, 0dB=1
    float norm = std::clamp((peakDb + 60.0f) / 60.0f, 0.0f, 1.0f);

    // Bar background
    auto barArea = area.removeFromBottom(area.getHeight() - 14);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(barArea.toFloat(), 2.0f);

    // Bar fill
    int fillH = static_cast<int>(
        static_cast<float>(barArea.getHeight()) * norm);
    auto filled = barArea.removeFromBottom(fillH);
    auto barCol = norm > 0.85f ? juce::Colour(0xFFFF4444)
                : norm > 0.6f  ? juce::Colour(0xFFFFAA33)
                               : kDebugColour;
    g.setColour(barCol.withAlpha(0.85f));
    g.fillRoundedRectangle(filled.toFloat(), 2.0f);

    // Label
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(label, area, juce::Justification::centred);
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

    // Knob section area (top part)
    auto bounds = getLocalBounds().reduced(8);
    bounds.removeFromTop(40);  // preset bar
    auto bottomSection = bounds.removeFromBottom(110); // debug section
    bounds.removeFromRight(40); // meter strip
    bounds.removeFromTop(4);

    static const juce::Colour kDiffuserColour {0xFFCC88DD};
    int totalCols = 15; // 5 delay + 3 reverb + 4 shared + 3 diffuser
    int colW = bounds.getWidth() / totalCols;

    // Delay section (5 cols)
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
    auto shArea = bounds.removeFromLeft(colW * 4);
    g.setColour(kSharedColour.withAlpha(0.08f));
    g.fillRoundedRectangle(shArea.toFloat().reduced(2), 8.0f);
    g.setColour(kSharedColour.withAlpha(0.3f));
    g.drawRoundedRectangle(shArea.toFloat().reduced(2), 8.0f, 1.0f);

    // Diffuser section (3 cols)
    auto diffArea = bounds;
    g.setColour(kDiffuserColour.withAlpha(0.08f));
    g.fillRoundedRectangle(diffArea.toFloat().reduced(2), 8.0f);
    g.setColour(kDiffuserColour.withAlpha(0.3f));
    g.drawRoundedRectangle(diffArea.toFloat().reduced(2), 8.0f, 1.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("MeltVerb",
               getLocalBounds().reduced(10, 6).removeFromTop(20),
               juce::Justification::right);

    // --- In/Out Level meters (right strip) ---
    auto meterStrip = getLocalBounds().reduced(8).removeFromRight(32);
    meterStrip.removeFromTop(44);
    meterStrip.removeFromBottom(110);

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

    // --- Debug section background ---
    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.fillRoundedRectangle(bottomSection.toFloat().reduced(2), 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(bottomSection.toFloat().reduced(2), 6.0f, 1.0f);

    // Debug section title
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Signal Debug",
               bottomSection.removeFromTop(16).reduced(8, 0),
               juce::Justification::left);

    // 6-point debug meters
    auto debugArea = bottomSection.reduced(8, 2);
    // Bypass buttons take left portion
    auto bypArea = debugArea.removeFromLeft(110);
    (void)bypArea; // layout handled in resized()

    int meterW = debugArea.getWidth() / 6;
    static const char* meterNames[kNumDebugMeters] = {
        "Input", "DelOut", "RevIn", "RevOut", "XFeed", "Output"
    };
    for (int i = 0; i < kNumDebugMeters; ++i)
    {
        auto col = debugArea.removeFromLeft(meterW);
        drawDebugMeter(g, col, meterNames[i], debugLevels_[i]);
    }
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

    // Bottom debug section
    auto bottomSection = bounds.removeFromBottom(110);

    // Main knob area
    bounds.removeFromRight(40); // meter strip

    int totalCols = 15;
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
    placeKnob(modDepthKnob,  modDepthLabel,  bounds.removeFromLeft(colW));

    // Diffuser (range combo + send + return)
    auto drArea = bounds.removeFromLeft(colW);
    diffRangeLabel.setBounds(drArea.removeFromTop(labelH));
    diffRangeBox.setBounds(drArea.reduced(4).removeFromTop(28));

    placeKnob(diffSendKnob,   diffSendLabel,   bounds.removeFromLeft(colW));
    placeKnob(diffReturnKnob, diffReturnLabel, bounds);

    // --- Debug section layout ---
    bottomSection.removeFromTop(16); // title
    auto debugLayout = bottomSection.reduced(8, 2);

    // Bypass buttons (left column)
    auto bypCol = debugLayout.removeFromLeft(110);
    // PingPongRandom Fix: 6ボタンを均等分割、残り全域への setBounds を廃止
    int btnH = bypCol.getHeight() / 6;
    bypDiffuserBtn.setBounds (bypCol.removeFromTop(btnH));
    bypDelayBtn.setBounds    (bypCol.removeFromTop(btnH));
    bypToneBtn.setBounds     (bypCol.removeFromTop(btnH));
    bypReverbBtn.setBounds   (bypCol.removeFromTop(btnH));
    bypCrossFeedBtn.setBounds(bypCol.removeFromTop(btnH));
    panRandomBtn.setBounds   (bypCol.removeFromTop(btnH));
}
