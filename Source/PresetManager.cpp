#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : apvts_(apvts)
{
}

juce::File PresetManager::getPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory)
        .getChildFile("MeltVerb")
        .getChildFile("Presets");
    dir.createDirectory();
    return dir;
}

juce::File PresetManager::getPresetFile(const juce::String& name) const
{
    return getPresetDirectory().getChildFile(name + ".xml");
}

void PresetManager::savePreset(const juce::String& name)
{
    if (name.isEmpty()) return;

    auto state = apvts_.copyState();
    auto xml = state.createXml();
    if (xml != nullptr)
    {
        getPresetFile(name).replaceWithText(xml->toString());
        currentPreset_ = name;
    }
}

void PresetManager::loadPreset(const juce::String& name)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile()) return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr)
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (tree.isValid())
        {
            apvts_.replaceState(tree);
            currentPreset_ = name;
        }
    }
}

void PresetManager::deletePreset(const juce::String& name)
{
    getPresetFile(name).deleteFile();
    if (currentPreset_ == name)
        currentPreset_.clear();
}

juce::StringArray PresetManager::getPresetList() const
{
    juce::StringArray list;
    for (auto& file : getPresetDirectory().findChildFiles(
             juce::File::findFiles, false, "*.xml"))
    {
        list.add(file.getFileNameWithoutExtension());
    }
    list.sort(true);
    return list;
}
