#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace MeltVerbParams {

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Delay ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_time", 1}, "Time",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_feedback", 1}, "Repeats",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.25f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_tone", 1}, "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_mix", 1}, "D.Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"delay_mode", 1}, "Mode", 0, 2, 0));

    // --- Reverb ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_decay", 1}, "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_damping", 1}, "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_mix", 1}, "R.Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // --- Shared ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"diffusion", 1}, "Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"cross_feed", 1}, "XFeed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mod_speed", 1}, "Mod Spd",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mod_depth", 1}, "Mod Dep",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // --- Diffuser control ---
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"diffuse_range", 1}, "Diff Range", 0, 2, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"diffuse_send", 1}, "Diff Send",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"diffuse_return", 1}, "Diff Ret",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // --- Bypass switches ---
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass_diffuser", 1}, "Diffuser", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass_delay", 1}, "Delay", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass_tone", 1}, "Tone", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass_reverb", 1}, "Reverb", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass_crossfeed", 1}, "XFeed", false));

    return { params.begin(), params.end() };
}

} // namespace MeltVerbParams
