#include "PluginProcessor.h"
#include "PluginEditor.h"

MeltVerbPlugin::MeltVerbPlugin()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", MeltVerbParams::createLayout()),
      presetManager(apvts)
{
    delayTimeParam     = apvts.getRawParameterValue("delay_time");
    delayFeedbackParam = apvts.getRawParameterValue("delay_feedback");
    delayToneParam     = apvts.getRawParameterValue("delay_tone");
    delayMixParam      = apvts.getRawParameterValue("delay_mix");
    delayModeParam     = apvts.getRawParameterValue("delay_mode");
    reverbDecayParam   = apvts.getRawParameterValue("reverb_decay");
    reverbDampParam    = apvts.getRawParameterValue("reverb_damping");
    reverbMixParam     = apvts.getRawParameterValue("reverb_mix");
    diffusionParam     = apvts.getRawParameterValue("diffusion");
    crossFeedParam     = apvts.getRawParameterValue("cross_feed");
    modSpeedParam      = apvts.getRawParameterValue("mod_speed");
    modDepthParam      = apvts.getRawParameterValue("mod_depth");

    diffuseRangeParam    = apvts.getRawParameterValue("diffuse_range");
    diffuseSendParam     = apvts.getRawParameterValue("diffuse_send");
    diffuseReturnParam   = apvts.getRawParameterValue("diffuse_return");

    bypassDiffuserParam  = apvts.getRawParameterValue("bypass_diffuser");
    bypassDelayParam     = apvts.getRawParameterValue("bypass_delay");
    bypassToneParam      = apvts.getRawParameterValue("bypass_tone");
    bypassReverbParam    = apvts.getRawParameterValue("bypass_reverb");
    bypassCrossFeedParam = apvts.getRawParameterValue("bypass_crossfeed");
}

void MeltVerbPlugin::prepareToPlay(double sampleRate, int)
{
    engine_.prepare(sampleRate);
    levelMeter.reset();
}

void MeltVerbPlugin::releaseResources()
{
    engine_.clear();
}

void MeltVerbPlugin::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numCh < 2 || numSamples == 0) return;

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);

    levelMeter.measureInput(L, R, numSamples);

    engine_.setDelayTime(delayTimeParam->load());
    engine_.setDelayFeedback(delayFeedbackParam->load());
    engine_.setDelayTone(delayToneParam->load());
    engine_.setDelayMix(delayMixParam->load());
    engine_.setDelayMode(static_cast<int>(delayModeParam->load()));
    engine_.setReverbDecay(reverbDecayParam->load());
    engine_.setReverbDamping(reverbDampParam->load());
    engine_.setReverbMix(reverbMixParam->load());
    engine_.setDiffusion(diffusionParam->load());
    engine_.setCrossFeed(crossFeedParam->load());
    engine_.setModSpeed(modSpeedParam->load());
    engine_.setModDepth(modDepthParam->load());

    engine_.setDiffuseRange(static_cast<int>(diffuseRangeParam->load()));
    engine_.setDiffuseSend(diffuseSendParam->load());
    engine_.setDiffuseReturn(diffuseReturnParam->load());

    engine_.setBypassDiffuser(bypassDiffuserParam->load() > 0.5f);
    engine_.setBypassDelay(bypassDelayParam->load() > 0.5f);
    engine_.setBypassTone(bypassToneParam->load() > 0.5f);
    engine_.setBypassReverb(bypassReverbParam->load() > 0.5f);
    engine_.setBypassCrossFeed(bypassCrossFeedParam->load() > 0.5f);

    engine_.process(L, R, numSamples);

    levelMeter.measureOutput(L, R, numSamples);
}

juce::AudioProcessorEditor* MeltVerbPlugin::createEditor()
{
    return new MeltVerbEditor(*this);
}

void MeltVerbPlugin::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MeltVerbPlugin::setStateInformation(const void* data,
                                          int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MeltVerbPlugin();
}
