/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct Params {
    float gain;
    bool phaseInv;
    bool phaseInvLeft;
    bool phaseInvRight;
    float balance;
    bool swapLR;
    bool mono;

    void update(const juce::AudioProcessorValueTreeState &apvts) {
        gain = apvts.getRawParameterValue("gain")->load();
        phaseInv = apvts.getRawParameterValue("phaseInv")->load();
        phaseInvLeft = apvts.getRawParameterValue("phaseInvLeft")->load();
        phaseInvRight = apvts.getRawParameterValue("phaseInvRight")->load();
        balance = apvts.getRawParameterValue("balance")->load();
        swapLR = apvts.getRawParameterValue("swapLR")->load();
        mono = apvts.getRawParameterValue("mono")->load();
    }
};

class LogicGainAudioProcessor : public juce::AudioProcessor {
public:
    LogicGainAudioProcessor();
    ~LogicGainAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts{*this, nullptr};

private:
    Params param;
    juce::dsp::Gain<float> gain;

    std::unique_ptr<juce::FileLogger> logger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogicGainAudioProcessor)
};
