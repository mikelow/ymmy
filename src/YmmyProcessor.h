#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class YmmyProcessor : public AudioProcessor {
public:
    YmmyProcessor();
    ~YmmyProcessor();

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

public:
    float noteOnVel;
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
//    std::unique_ptr<juce::AudioProcessorValueTreeState> state;
  AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

//  Synthesiser synth;
  AudioProcessorValueTreeState valueTreeState;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YmmyProcessor)
};

