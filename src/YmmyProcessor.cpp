#include "YmmyProcessor.h"
#include "YmmyEditor.h"
#include "synths/Synth.h"
#include "synths/fluidsynth/FluidSynthSynth.h"
#include "MidiConstants.h"

YmmyProcessor::YmmyProcessor()
        : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          vts{*this, nullptr, juce::Identifier ("YmmySettings"), createParameterLayout()},
          channelGroup(0), currentChannel(1) {

  addSynth(std::make_unique<FluidSynthSynth>(vts));

  FluidSynthSynth::updateValueTreeState(vts);
}

YmmyProcessor::~YmmyProcessor() {
}

template <typename T>
void callCreateParameterLayout() {
  T::createParameterLayout();
}

AudioProcessorValueTreeState::ParameterLayout YmmyProcessor::createParameterLayout() {
//  return {
//std::make_unique<juce::AudioParameterFloat>("midiVolume", "MIDI Volume", 0.0f, 127.0f, 1.0f),
//std::make_unique<juce::AudioParameterBool>("invertPhase", "Invert Phase", false)
//  };

  // Root layout for the parameters
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  std::make_unique<AudioParameterInt>("channel", "currently selected channel", 0, maxChannels, MidiConstants::midiMinValue, "Channel" );


  auto fluidSynthParams = FluidSynthSynth::createParameterGroup();

//  std::unique_ptr<AudioParameterInt> params[] {
//      // SoundFont 2.4 spec section 7.2: zero through 127, or 128.
//      std::make_unique<AudioParameterInt>("bank", "which bank is selected in the soundfont", MidiConstants::midiMinValue, 128, MidiConstants::midiMinValue, "Bank" ),
//      // note: banks may be sparse, and lack a 0th preset. so defend against this.
//      std::make_unique<AudioParameterInt>("preset", "which patch (aka patch, program, instrument) is selected in the soundfont", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "Preset" ),
//      std::make_unique<AudioParameterInt>("midiVolume", "volume envelope attack time", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "A" ),
//      std::make_unique<AudioParameterInt>("attackRate", "volume envelope attack time", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "A" ),
//      std::make_unique<AudioParameterInt>("decayRate", "volume envelope sustain attentuation", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "D" ),
//      std::make_unique<AudioParameterInt>("sustainLevel", "volume envelope decay time", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "S" ),
//      std::make_unique<AudioParameterInt>("releaseRate", "volume envelope release time", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "R" ),
//      std::make_unique<AudioParameterInt>("filterCutOff", "low-pass filter cut-off frequency", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "Cut" ),
//      std::make_unique<AudioParameterInt>("filterResonance", "low-pass filter resonance attentuation", MidiConstants::midiMinValue, MidiConstants::midiMaxValue, MidiConstants::midiMinValue, "Res" ),
//  };
//
//  return {
//      make_move_iterator(begin(params)),
//      make_move_iterator(end(params))
//  };


//  std::unique_ptr<AudioParameterInt> params[]{
//      std::make_unique<AudioParameterInt>("midiVolume", "midi volume",
//                                          MidiConstants::midiMinValue, MidiConstants::midiMaxValue,
//                                          MidiConstants::midiMinValue, "Volume"),
//      std::make_unique<AudioParameterInt>("attackRate", "volume envelope attack time",
//                                          MidiConstants::midiMinValue, MidiConstants::midiMaxValue,
//                                          MidiConstants::midiMinValue, "A"),
//      std::make_unique<AudioParameterInt>("decayRate", "volume envelope sustain attentuation",
//                                          MidiConstants::midiMinValue, MidiConstants::midiMaxValue,
//                                          MidiConstants::midiMinValue, "D"),
//      std::make_unique<AudioParameterInt>("sustainLevel", "volume envelope decay time",
//                                          MidiConstants::midiMinValue, MidiConstants::midiMaxValue,
//                                          MidiConstants::midiMinValue, "S"),
//      std::make_unique<AudioParameterInt>("releaseRate", "volume envelope release time",
//                                          MidiConstants::midiMinValue, MidiConstants::midiMaxValue,
//                                          MidiConstants::midiMinValue, "R"),
//////      std::make_unique<juce::AudioParameterFloat>("midiVolume", "Release Rate", 0.0f, 127.0f, 1.0f),
//////      std::make_unique<juce::AudioParameterFloat>("attackRate", "Release Rate", 0.0f, 127.0f, 1.0f),
//////      std::make_unique<juce::AudioParameterFloat>("sustainLevel", "Release Rate", 0.0f, 127.0f, 1.0f),
//////      std::make_unique<juce::AudioParameterFloat>("decayRate", "Release Rate", 0.0f, 127.0f, 1.0f),
//////      std::make_unique<juce::AudioParameterFloat>("releaseRate", "Release Rate", 0.0f, 127.0f, 1.0f)
////
////
//  };
////
//  layout.add(
//            make_move_iterator(begin(params)),
//            make_move_iterator(end(params))
//            );


  // Create groups for each synth type
//  auto fluidSynthParams = std::make_unique<juce::AudioProcessorParameterGroup>("fluidsynth", "FluidSynth", "|");
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("fs.midiVolume", "MIDI Volume", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("synthOneParam2", "Parameter 2", 0.0f, 127.0f, 64.0f));

//  auto ym2151Params = std::make_unique<juce::AudioProcessorParameterGroup>("ym2151", "YM2151", "|");
//  ym2151Params->addChild(std::make_unique<juce::AudioParameterFloat>("ym2151_synthTwoParam1", "Parameter 1", 0.0f, 1.0f, 0.5f));
//  ym2151Params->addChild(std::make_unique<juce::AudioParameterFloat>("synthTwoParam2", "Parameter 2", 0.0f, 127.0f, 64.0f));

  // Add groups to the layout
  layout.add(std::move(fluidSynthParams));
//  layout.add(std::move(ym2151Params));

  return layout;
}

void YmmyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();

    MidiMessage m;

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Map to hold MidiBuffers for each Synth
    std::unordered_map<Synth*, juce::MidiBuffer> synthMidiBuffers;

    // Initialize MidiBuffers for each Synth
    for (auto& synth : synths) {
      synthMidiBuffers[synth.get()] = juce::MidiBuffer();
    }

    // Iterate through all MidiMessages
    juce::MidiBuffer::Iterator it(midiMessages);
    juce::MidiMessage message;
    int samplePosition;
    while (it.getNextEvent(message, samplePosition)) {
//      DEBUG_PRINT(m.getDescription());

      int channel = message.getChannel() + (channelGroup * 16);
      if (synths.size() == 1) {
        synthMidiBuffers[synths[0].get()].addEvent(message, samplePosition);
      } else if (channelToSynthMap.count(channel) > 0) {
        // Add the message to the corresponding Synth's MidiBuffer
        // We might consider checking isMetaEvent() and adding to all synths
        Synth* synth = channelToSynthMap[channel];
        synthMidiBuffers[synth].addEvent(message, samplePosition);
      }
//      else if (message.isSysEx()) {
//        auto sysexData = m.getSysExData();
//        auto sysexDataSize = m.getSysExDataSize();
//        if (sysexData[0] == 0x7D && sysexData[1] < 0x7E) {
//          channelGroup = static_cast<int>(sysexData[1]);
//          //          auto wrappedMsg = juce::MidiMessage(sysexData + 2, m.getSysExDataSize() - 2, 0); handleMidiMessage(wrappedMsg);
//        }
//      }
    }

    // Process each Synth with its corresponding MidiBuffer
    for (auto& synth : synths) {
      synth->processBlock(buffer, synthMidiBuffers[synth.get()]);
    }
}

//void YmmyProcessor::routeMidiMessage()

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
  if (channelToSynthMap.count(currentChannel) == 0) {
    return 1;
  }
  auto synth = channelToSynthMap[currentChannel];
  return synth->getCurrentProgram();
}

int YmmyProcessor::getCurrentProgram() {
  if (channelToSynthMap.count(currentChannel) == 0) {
    return 0;
  }
  auto synth = channelToSynthMap[currentChannel];
  return synth->getCurrentProgram();
}

void YmmyProcessor::setCurrentProgram (int index) {
  if (channelToSynthMap.count(index) == 0) {
    return;
  }
  auto synth = channelToSynthMap[index];
  synth->setCurrentProgram(index);
}

const juce::String YmmyProcessor::getProgramName (int index) {
  return {};
}

void YmmyProcessor::changeProgramName (int index, const juce::String& newName) {
}

//==============================================================================
void YmmyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
  keyboardState.reset();
  for (const auto& synth : synths) {
    synth->prepareToPlay(sampleRate, samplesPerBlock);
  }
}

void YmmyProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
  keyboardState.reset();
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

//==============================================================================
bool YmmyProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* YmmyProcessor::createEditor() {
    return new YmmyEditor (*this, vts);
}

void YmmyProcessor::getStateInformation (juce::MemoryBlock& destData) {
  auto state = vts.copyState();
  std::unique_ptr<juce::XmlElement> xml (state.createXml());
  copyXmlToBinary (*xml, destData);

  DBG(xml->toString());
}

void YmmyProcessor::setStateInformation (const void* data, int sizeInBytes) {
//  std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    std::shared_ptr<XmlElement> xmlState{getXmlFromBinary(data, sizeInBytes)};

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(vts.state.getType())) {
      DBG(xmlState->toString());
      vts.replaceState(juce::ValueTree::fromXml(*xmlState));
//      printf("vts.state before:\n");
//      printf("%s", vts.state.toXmlString().toStdString().c_str());

//      DBG(vts.state.toXmlString());
//      vts.state.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xmlState), nullptr);
//      printf("vts.state after:\n");
//      printf("%s", vts.state.toXmlString().toStdString().c_str());
      vts.state.getChildWithName("soundFont").sendPropertyChangeMessage("path");

//      XmlElement* params{xmlState->getChildByName("params")};
//      if (params) {
//        for (auto* param : getParameters()) {
//          if (auto* p = dynamic_cast<AudioProcessorParameterWithID*>(param)) {
//            p->setValueNotifyingHost(static_cast<float>(params->getDoubleAttribute(p->paramID, p->getValue())));
//          }
//        }
//      }
    }
  }
}

//void YmmyProcessor::getStateInformation (MemoryBlock& destData)
//{
//  // You should use this method to store your parameters in the memory block.
//  // You could do that either as raw data, or use the XML or ValueTree classes
//  // as intermediaries to make it easy to save and load complex data.
//
//  // Create an outer XML element..
//  XmlElement xml{"YmmySettings"};
//
//  // Store the values of all our parameters, using their param ID as the XML attribute
//  XmlElement* params{xml.createNewChildElement("params")};
//  for (auto* param : getParameters()) {
//    if (auto* p = dynamic_cast<AudioProcessorParameterWithID*> (param)) {
//      params->setAttribute(p->paramID, p->getValue());
//    }
//  }
//  {
//    ValueTree tree{vts.state.getChildWithName("soundFont")};
//    XmlElement* newElement{xml.createNewChildElement("soundFont")};
//    {
//      String value = tree.getProperty("path", "");
//      newElement->setAttribute("path", value);
//      //            String memFileVal = tree.getProperty("memfile", var(nullptr, 0));
//      //            newElement->setAttribute("memfile", memFileVal);
//    }
//  }
//
////  DEBUG_PRINT(xml.createDocument("",false,false));
//
//  copyXmlToBinary(xml, destData);
//}
//
//void YmmyProcessor::setStateInformation (const void* data, int sizeInBytes)
//{
//  // You should use this method to restore your parameters from this memory block,
//  // whose contents will have been created by the getStateInformation() call.
//  // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
//  std::shared_ptr<XmlElement> xmlState{getXmlFromBinary(data, sizeInBytes)};
////  DEBUG_PRINT(xmlState->createDocument("",false,false));
//
//  if (xmlState.get() != nullptr) {
//    // make sure that it's actually our type of XML object..
//    if (xmlState->hasTagName(vts.state.getType())) {
//      {
//        XmlElement* xmlElement{xmlState->getChildByName("soundFont")};
//        if (xmlElement) {
//          ValueTree tree{vts.state.getChildWithName("soundFont")};
//          Value value{tree.getPropertyAsValue("path", nullptr)};
//          value = xmlElement->getStringAttribute("path", value.getValue());
//        }
//      }
//      XmlElement* params{xmlState->getChildByName("params")};
//      if (params) {
//        for (auto* param : getParameters()) {
//          if (auto* p = dynamic_cast<AudioProcessorParameterWithID*>(param)) {
//            p->setValueNotifyingHost(static_cast<float>(params->getDoubleAttribute(p->paramID, p->getValue())));
//          }
//        }
//      }
//    }
//  }
//}

void YmmyProcessor::addSynth(std::unique_ptr<Synth> synth) {
  // Add synth to the list and update the MIDI channel map
//  synths.push_back
  synths.push_back(std::move(synth));
}

void YmmyProcessor::removeSynth(Synth* synth) {
  // Remove synth from the list and update the MIDI channel map
}