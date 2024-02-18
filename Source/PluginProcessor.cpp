/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include <JucePluginDefines.h>
#include "PluginEditor.h"

LogicGainAudioProcessor::LogicGainAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
    logger = std::make_unique<juce::FileLogger>(
        juce::File("/Users/xuchen/tmp/LogicGain.log"), "LogicGain",
        1024 * 1024);

    logger->logMessage("LogicGainAudioProcessor::LogicGainAudioProcessor() " +
                       std::to_string(getNumInputChannels()));

    apvts.createAndAddParameter(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain",
        juce::NormalisableRange<float>(-96.0f, 24.0f, 0.1f, 1.0f), 0.0f, "dB"));

    if (getTotalNumInputChannels() == 1) {
        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
            "phaseInv", "Phase Invert", false));
    } else if (getTotalNumInputChannels() == 2) {
        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
            "phaseInvLeft", "Phase Left", false));

        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
            "phaseInvRight", "Phase Right", false));

        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterFloat>(
            "balance", "Balance",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f, 1.0f), 0.0f,
            "%"));

        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
            "swapLR", "Swap L/R", false));

        apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
            "mono", "Make Mono", false));
    }
    apvts.state = juce::ValueTree("savedParams");
}

LogicGainAudioProcessor::~LogicGainAudioProcessor() {}

const juce::String LogicGainAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool LogicGainAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool LogicGainAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool LogicGainAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double LogicGainAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int LogicGainAudioProcessor::getNumPrograms() {
    return 1;  // NB: some hosts don't cope very well if you tell them there are
               // 0 programs, so this should be at least 1, even if you're not
               // really implementing programs.
}

int LogicGainAudioProcessor::getCurrentProgram() {
    return 0;
}

void LogicGainAudioProcessor::setCurrentProgram(int index) {}

const juce::String LogicGainAudioProcessor::getProgramName(int index) {
    return {};
}

void LogicGainAudioProcessor::changeProgramName(int index,
                                                const juce::String &newName) {}

void LogicGainAudioProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;

    gain.prepare(spec);
}

void LogicGainAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LogicGainAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void LogicGainAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                           juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumInputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto ctx = juce::dsp::ProcessContextReplacing<float>(block);
    gain.setGainDecibels(param.gain);
    gain.process(ctx);

    bool isMono = totalNumInputChannels == 1;
    // param.update(apvts, isMono);

    if (buffer.getNumChannels() == 2) {
        auto *channelLeft = buffer.getWritePointer(0);
        auto *channelRight = buffer.getWritePointer(1);

        float coefLeft = 1.0f;
        float coefRight = 1.0f;

        if (param.phaseInvLeft) {
            coefLeft *= -1;
        }
        if (param.phaseInvRight) {
            coefRight *= -1;
        }
        if (param.balance > 0) {
            coefLeft *= 1.0f - param.balance / 100.0f;
        } else {
            coefRight *= 1.0f + param.balance / 100.0f;
        }

        for (auto i = 0; i < buffer.getNumSamples(); ++i) {
            channelLeft[i] *= coefLeft;
            channelRight[i] *= coefRight;

            if (param.mono) {
                float mono = (channelLeft[i] + channelRight[i]) / 2.0f;
                channelLeft[i] = mono;
                channelRight[i] = mono;
            }
            if (!param.mono && param.swapLR) {
                std::swap(channelLeft[i], channelRight[i]);
            }
        }
    } else if (buffer.getNumChannels() == 1) {
        auto *channel = buffer.getWritePointer(0);
        if (param.phaseInv) {
            juce::FloatVectorOperations::multiply(channel, -1.0f,
                                                  buffer.getNumSamples());
        }
    }
}

bool LogicGainAudioProcessor::hasEditor() const {
    return true;  // (change this to false if you choose to not supply an
                  // editor)
}

juce::AudioProcessorEditor *LogicGainAudioProcessor::createEditor() {
    // return new LogicGainAudioProcessorEditor(*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

void LogicGainAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    std::unique_ptr<juce::XmlElement> outputXml(apvts.state.createXml());
    copyXmlToBinary(*outputXml, destData);
}

void LogicGainAudioProcessor::setStateInformation(const void *data,
                                                  int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> inputXml(
        getXmlFromBinary(data, sizeInBytes));
    if (inputXml.get() != nullptr) {
        if (inputXml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*inputXml));
        }
    }
}

// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new LogicGainAudioProcessor();
}

// void LogicGainAudioProcessor::setPlayConfigDetails(int newNumInputChannels,
//                                                    int newNumOutputChannels,
//                                                    double newSampleRate,
//                                                    int newBlockSize) {
//     // Store the number of channels for later use
//     int numInputChannels = newNumInputChannels;
//     int numOutputChannels = newNumOutputChannels;

//     // Create and configure parameters based on the channel configuration
//     apvts.createAndAddParameter(std::make_unique<juce::AudioParameterFloat>(
//         "gain", "Gain",
//         juce::NormalisableRange<float>(-96.0f, 24.0f, 0.1f, 1.0f), 0.0f,
//         "dB"));

//     if (numInputChannels == 1) {
//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
//             "phaseInv", "Phase Invert", false));
//     } else if (numInputChannels == 2) {
//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
//             "phaseInvLeft", "Phase Left", false));

//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
//             "phaseInvRight", "Phase Right", false));

//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterFloat>(
//             "balance", "Balance",
//             juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f, 1.0f),
//             0.0f,
//             "%"));

//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
//             "swapLR", "Swap L/R", false));

//         apvts.createAndAddParameter(std::make_unique<juce::AudioParameterBool>(
//             "mono", "Make Mono", false));
//     }
//     apvts.state = juce::ValueTree("savedParams");
// }