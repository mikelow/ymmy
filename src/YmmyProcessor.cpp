#include "YmmyProcessor.h"
#include "YmmyEditor.h"

YmmyProcessor::YmmyProcessor()
        : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          valueTreeState{*this, nullptr, juce::Identifier ("YmmySettings"), createParameterLayout()} {
}

YmmyProcessor::~YmmyProcessor() {
}

AudioProcessorValueTreeState::ParameterLayout YmmyProcessor::createParameterLayout() {
//  return {
//std::make_unique<juce::AudioParameterFloat>("midiVolume", "MIDI Volume", 0.0f, 127.0f, 1.0f),
//std::make_unique<juce::AudioParameterBool>("invertPhase", "Invert Phase", false)
//  };

  // Root layout for the parameters
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Create groups for each synth type
  auto fluidSynthParams = std::make_unique<juce::AudioProcessorParameterGroup>("fluidsynth", "FluidSynth", "|");
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("fs.midiVolume", "MIDI Volume", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("synthOneParam2", "Parameter 2", 0.0f, 127.0f, 64.0f));

  auto ym2151Params = std::make_unique<juce::AudioProcessorParameterGroup>("ym2151", "YM2151", "|");
  ym2151Params->addChild(std::make_unique<juce::AudioParameterFloat>("ym2151_synthTwoParam1", "Parameter 1", 0.0f, 1.0f, 0.5f));
  ym2151Params->addChild(std::make_unique<juce::AudioParameterFloat>("synthTwoParam2", "Parameter 2", 0.0f, 127.0f, 64.0f));

  // Add groups to the layout
  layout.add(std::move(fluidSynthParams));
  layout.add(std::move(ym2151Params));

  return layout;
}

void YmmyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();

    juce::MidiBuffer processedMidi;

//    auto rawParamVal = valueTreeState.getRawParameterValue ("fs.midiVolume");
//    printf("midiVolume: %f\n", rawParamVal->load());
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;

        if (message.isNoteOn())
        {
            message = juce::MidiMessage::noteOn (message.getChannel(),
                                                 message.getNoteNumber(),
                                                 (juce::uint8) noteOnVel);
        }

        processedMidi.addEvent (message, time);
    }

    midiMessages.swapWith (processedMidi);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new YmmyProcessor();
}


/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

//==============================================================================
const juce::String YmmyProcessor::getName() const {
  return JucePlugin_Name;
}

bool YmmyProcessor::acceptsMidi() const {
  return true;
}

bool YmmyProcessor::producesMidi() const {
  return false;
}

bool YmmyProcessor::isMidiEffect() const {
  return false;
}

double YmmyProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int YmmyProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int YmmyProcessor::getCurrentProgram() {
    return 0;
}

void YmmyProcessor::setCurrentProgram (int index) {
}

const juce::String YmmyProcessor::getProgramName (int index) {
    return {};
}

void YmmyProcessor::changeProgramName (int index, const juce::String& newName) {
}

//==============================================================================
void YmmyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
//    synth.setCurrentPlaybackSampleRate (sampleRate);

}

void YmmyProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool YmmyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
}
#endif

//void YmmyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
//{
//    juce::ScopedNoDenormals noDenormals;
//    auto totalNumInputChannels  = getTotalNumInputChannels();
//    auto totalNumOutputChannels = getTotalNumOutputChannels();
//
//    // In case we have more outputs than inputs, this code clears any output
//    // channels that didn't contain input data, (because these aren't
//    // guaranteed to be empty - they may contain garbage).
//    // This is here to avoid people getting screaming feedback
//    // when they first compile a plugin, but obviously you don't need to keep
//    // this code if your algorithm always overwrites all the output channels.
//    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
//        buffer.clear (i, 0, buffer.getNumSamples());
//
//    // This is the place where you'd normally do the guts of your plugin's
//    // audio processing...
//    // Make sure to reset the state if your inner loop is processing
//    // the samples and the outer loop is handling the channels.
//    // Alternatively, you can process the samples with the channels
//    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
//}

//==============================================================================
bool YmmyProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* YmmyProcessor::createEditor() {
    return new YmmyEditor (*this, valueTreeState);
}

//==============================================================================
//void YmmyProcessor::getStateInformation (juce::MemoryBlock& destData)
//{
//    // You should use this method to store your parameters in the memory block.
//    // You could do that either as raw data, or use the XML or ValueTree classes
//    // as intermediaries to make it easy to save and load complex data.
//}
//
//void YmmyProcessor::setStateInformation (const void* data, int sizeInBytes)
//{
//    // You should use this method to restore your parameters from this memory block,
//    // whose contents will have been created by the getStateInformation() call.
//}


void YmmyProcessor::getStateInformation (juce::MemoryBlock& destData) {
  auto state = valueTreeState.copyState();
  std::unique_ptr<juce::XmlElement> xml (state.createXml());
  copyXmlToBinary (*xml, destData);
}

void YmmyProcessor::setStateInformation (const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName (valueTreeState.state.getType()))
      valueTreeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}