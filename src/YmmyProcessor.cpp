#include "YmmyProcessor.h"
#include "YmmyEditor.h"
#include "synths/Synth.h"
#include "synths/fluidsynth/FluidSynthSynth.h"
#include "synths/ym2151/YM2151Synth.h"
#include "MidiConstants.h"
#include "utility.h"

YmmyProcessor::YmmyProcessor()
        : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          vts{*this, nullptr, juce::Identifier ("YmmySettings"), createParameterLayout()},
          channelGroup(0), settings{0} {

  addSynth(std::make_unique<FluidSynthSynth>(vts));
  addSynth(std::make_unique<YM2151Synth>(this, vts));

  // setSynthForChannel(0, 0, YM2151);
  // setSynthForChannel(0, 1, FluidSynth);
//  addSettingsToVTS();
//  FluidSynthSynth::getInitialChildValueTree();
//  FluidSynthSynth::updateValueTreeState(vts);
}

YmmyProcessor::~YmmyProcessor() {
  synths.clear();
}

template <typename T>
void callCreateParameterLayout() {
  T::createParameterLayout();
}

AudioProcessorValueTreeState::ParameterLayout YmmyProcessor::createParameterLayout() {

  // Root layout for the parameters
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

//  std::make_unique<AudioParameterInt>("channel", "currently selected channel", 0, maxChannels, MidiConstants::midiMinValue, "Channel" );

  auto fluidSynthParams = FluidSynthSynth::createParameterGroup();
  auto ym2151SynthParams = YM2151Synth::createParameterGroup();
  // Add groups to the layout
  layout.add(std::move(fluidSynthParams));
  layout.add(std::move(ym2151SynthParams));
//  layout.add(std::move(ym2151Params));

  return layout;
}


// returns true if sysex was handled, or false if it was ignored
bool YmmyProcessor::handleSysex(
  MidiMessage& message,
  int samplePosition,
  std::unordered_map<Synth*, juce::MidiBuffer>& synthMidiBuffers
) {
  auto sysexData = message.getSysExData();
  auto sysexDataSize = message.getSysExDataSize();

  if (sysexData[0] != 0x7D) {
    return false;
  }
  uint8_t opcode = sysexData[1];

  // If it's a wrapped MIDI message, decode it
  if (opcode >= 0 && opcode < 16) {
    if (synths.size() == 1) {
      synthMidiBuffers[synths[0].get()].addEvent(message, samplePosition);
    } else {
      auto channelGroup = static_cast<int>(sysexData[1]);
      auto wrappedMsg = juce::MidiMessage(sysexData + 2, message.getSysExDataSize() - 2, 0);
      auto channel = wrappedMsg.getChannel() - 1;

      if (auto it = channelToSynthMap.find(channel + channelGroup * 16); it != channelToSynthMap.end() && it->second != nullptr) {
        // Add the message to the corresponding Synth's MidiBuffer
        // We might consider checking isMetaEvent() and adding to all synths
        Synth* synth = it->second;
        synthMidiBuffers[synth].addEvent(message, samplePosition);
      } else {
        DBG("Event channel without assigned synth");
      }
    }
    // processMidiMessage(wrappedMsg);
    return true;
  }

  switch (opcode) {
    case 0x10: {  // SF2 send start {
      int bytesRead = 0;
      incomingFileSize = read6BitVariableLengthQuantity(sysexData+3, sysexDataSize-3, bytesRead);
      incomingFile.reset();
      break;
    }
    case 0x11: {
      if (incomingFileSize == 0) {
        break;
      }
      auto destBuf = new uint8_t[static_cast<unsigned long>(sysexDataSize + 8)];
      int inOffset = 3;     // The first three sysex bytes are manufacturer ID and command, and file type, so skip past them
      int outOffset = 0;
      while (inOffset < sysexDataSize) {
        read7BitChunk(sysexData+inOffset, destBuf+outOffset);
        inOffset += 8;
        outOffset += 7;
      }
      incomingFileSize -= outOffset;
      if (incomingFileSize <= 0) {
        // shave off any extraneous bytes that weren't read
        outOffset += incomingFileSize;
        incomingFileSize = 0;
      }
      incomingFile.append(destBuf, static_cast<size_t>(outOffset));
      // We should probably load the SF2 if incomingSF2Size == 0

      if (incomingFileSize > 0) {
        delete[] destBuf;
        break;
      }

      SynthFileType fileType = static_cast<SynthFileType>(sysexData[2]);
      SynthType synthType = fileTypeToSynthType(fileType);
      if (auto synth = synthTypeToSynth(synthType)) {
        synth->receiveFile(incomingFile, fileType);
      }

      delete[] destBuf;
      incomingFile.reset();
      break;
    }


    // case 0x10:
    // case 0x11: {
    //   // For send file opcodes, we determine the destination synth using the file type byte
    //   SynthFileType fileType = static_cast<SynthFileType>(sysexData[2]);
    //   switch (fileType) {
    //     case SoundFont2:
    //       if (auto fluidSynth = synthTypeToSynth(FluidSynth)) {
    //         synthMidiBuffers[fluidSynth].addEvent(message, samplePosition);
    //       }
    //       break;
    //     case YM2151_OPM:
    //       if (auto ym2151Synth = synthTypeToSynth(YM2151)) {
    //         synthMidiBuffers[ym2151Synth].addEvent(message, samplePosition);
    //       }
    //       break;
    //   }
    //   break;
    // }
    case 0x7F: {
      if (auto fluidSynth = synthTypeToSynth(FluidSynth)) {
        synthMidiBuffers[fluidSynth].addEvent(message, samplePosition);
      }
      break;
    }
    // assign synth to channel/group
    case 0x7E: {
      uint8_t chan = sysexData[2];
      uint8_t group = sysexData[3];
      std::string name(reinterpret_cast<const char*>(&sysexData[4]));
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);
      auto synthOpt = getValueFromMap(stringToSynthType, name);

      if (synthOpt.has_value()) {
        setSynthForChannel(group, chan, synthOpt.value());
      } else {
        std::cout << "Attempting to set Synth for channel with invalid SynthType string: " << name << std::endl;
      }
      return true;
    }
  }
  return false;
}

void YmmyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();

    MidiMessage m;

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Map to hold MidiBuffers for each Synth
    std::unordered_map<Synth*, juce::MidiBuffer> synthMidiBuffers;

    // Initialize MidiBuffers for each Synth
    for (auto& synth : synths) {
      if (synthMidiBuffers.find(synth.get()) == synthMidiBuffers.end()) {
        synthMidiBuffers[synth.get()] = juce::MidiBuffer();
      }
    }

    // Iterate through all MidiMessages
    juce::MidiBuffer::Iterator it(midiMessages);
    juce::MidiMessage message;
    int samplePosition;
    while (it.getNextEvent(message, samplePosition)) {
//      DEBUG_PRINT(m.getDescription());
      if (message.isSysEx()) {
        if (handleSysex(message, samplePosition, synthMidiBuffers)) {
          continue;
        }
      } else {
        auto channel = message.getChannel() - 1;
        if (auto it = channelToSynthMap.find(channel + channelGroup * 16); it != channelToSynthMap.end() && it->second != nullptr) {
          Synth* synth = it->second;
          synthMidiBuffers[synth].addEvent(message, samplePosition);
        }
      }
    }

    // Process each Synth with its corresponding MidiBuffer
    for (auto& synth : synths) {
      auto midiBuffer = synthMidiBuffers[synth.get()];
      synth->processBlock(buffer, midiBuffer);
      midiBuffer.clear();
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
  if (channelToSynthMap.count(settings.selectedChannel) == 0) {
    return 1;
  }
  auto synth = channelToSynthMap[settings.selectedChannel];
  return synth->getCurrentProgram();
}

int YmmyProcessor::getCurrentProgram() {
  if (channelToSynthMap.count(settings.selectedChannel) == 0) {
    return 0;
  }
  auto synth = channelToSynthMap[settings.selectedChannel];
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
  return getSelectedSynth()->getProgramName(index);
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

  DBG(vts.state.toXmlString());

  auto state = vts.copyState();
//  state.removeAllChildren(nullptr);
//  state.removeAllProperties(nullptr);
  std::unique_ptr<juce::XmlElement> xml (state.createXml());
  copyXmlToBinary (*xml, destData);

  DBG(xml->toString());
}

void YmmyProcessor::setStateInformation (const void* data, int sizeInBytes) {
//  std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    std::shared_ptr<XmlElement> xmlState{getXmlFromBinary(data, sizeInBytes)};

    DBG(vts.state.toXmlString());

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(vts.state.getType())) {
//      DBG(xmlState->toString());

      ValueTree xmlTree = juce::ValueTree::fromXml(*xmlState);

//      createParameterLayout();
      auto rootTree = getInitialChildValueTree();
      auto fluidTree = FluidSynthSynth::getInitialChildValueTree();
      auto ym2151Tree = YM2151Synth::getInitialChildValueTree();

      for (auto initTree: { rootTree, fluidTree, ym2151Tree}) {
        for (auto i = initTree.getNumChildren(); --i >= 0;) {
          auto child = initTree.getChild(i);  //.createCopy();
//          if (xmlTree.hasType(child.getType())) {
          auto xmlTreeChild = xmlTree.getChildWithName(child.getType());
//          if (xmlTree.getChildWithName(child.getType()).isValid()) {
          if (xmlTreeChild.isValid()) {
//            int numInitChildProps = child.getNumProperties();
//            for (auto propIx = 0; propIx < numInitChildProps; ++propIx) {
//              auto initChildPropName = child.getPropertyName(propIx);
//              if (xmlTreeChild.hasProperty(initChildPropName)) {
//                auto xmlTreeChildProp = xmlTreeChild.getProperty(initChildPropName);
//              }
//              if
//            }
            continue;
          }
          //          initTree.removeChild(child, nullptr);
          xmlTree.addChild(child.createCopy(), -1, nullptr);
        }
      }

//      juce::String path = xmlTree.getChildWithName("soundFont").getProperty("path");
//      vts.state.addChild({ "soundFont", { { "path", path }, }, {} }, -1, nullptr);
//      vts.state.getChildWithName("soundFont").sendPropertyChangeMessage("path");
      // Force an update to selectedChannel before anything else
      int group = xmlTree.getChildWithName("settings").getProperty("selectedGroup");
      int chan = xmlTree.getChildWithName("settings").getProperty("selectedChannel");
      vts.state.addChild({ "settings", { { "selectedGroup", group }, { "selectedChannel", chan }, }, {} }, -1, nullptr);
      vts.state.getChildWithName("settings").sendPropertyChangeMessage("selectedGroup");
      vts.state.getChildWithName("settings").sendPropertyChangeMessage("selectedChannel");
      settings.selectedGroup = group;
      settings.selectedChannel = chan;

      vts.replaceState(xmlTree);

      DBG(vts.state.toXmlString());
      vts.state.getChildWithName("soundFont").sendPropertyChangeMessage("path");
      vts.state.getChildWithName("opmFile").sendPropertyChangeMessage("path");
    }
  }
}

const ValueTree YmmyProcessor::getInitialChildValueTree() {
  return
    { "ignoredRoot",
      {},
      {
        { "settings",
          {
            { "selectedGroup", 0 },
            { "selectedChannel", 0 },
          }, {}
        },
      }
    };
}

void YmmyProcessor::setVtsSettingsProperty(const juce::String& propertyName, const var& newValue) {
  auto settingsVt = vts.state.getChildWithName(Identifier("settings"));
  Value value{settingsVt.getPropertyAsValue(Identifier(propertyName), nullptr)};
//  auto propName = settingsVt.getPropertyName(0);
//  auto test = settingsVt.getProperty(Identifier(propertyName));
//  auto test2 = settingsVt.getPropertyAsValue(Identifier(propertyName), nullptr);
//  int test3 = test2.getValue();
//  printf("propName(0)\n");
//  printf("%s\n", propName.toString().toStdString().c_str());
//  printf("TEST\n");
//  printf("%s\n", test.toString().toStdString().c_str());
//  printf("TEST2\n");
//  printf("%d\n", test3);
//  printf("value\n");
//  printf("%s\n", value.toString().toStdString().c_str());
  value.setValue(newValue);
}

void YmmyProcessor::setSelectedGroup(int group) {
  if (group < 0 || group >= 16) {
    return;
  }
  settings.selectedGroup = group;
  setVtsSettingsProperty("selectedGroup", group);
}

void YmmyProcessor::setSelectedChannel(int chan) {
  if (chan < 0 || chan >= 16) {
    return;
  }
  settings.selectedChannel = chan;
  setVtsSettingsProperty("selectedChannel", chan);
}

void YmmyProcessor::incrementChannel() {
  setSelectedChannel(settings.selectedChannel + 1);
}
void YmmyProcessor::decrementChannel() {
  setSelectedChannel(settings.selectedChannel - 1);
}

void YmmyProcessor::addSynth(std::unique_ptr<Synth> synth) {
  // Add synth to the list and update the MIDI channel map
  synths.push_back(std::move(synth));
}

void YmmyProcessor::removeSynth(Synth* synth) {
  // Remove synth from the list and update the MIDI channel map

}

template <typename T>
T* YmmyProcessor::findSynth(const std::vector<std::unique_ptr<Synth>>& synths) {
  auto it = std::find_if(synths.begin(), synths.end(), [](const std::unique_ptr<Synth>& synthPtr) -> bool {
    return dynamic_cast<T*>(synthPtr.get()) != nullptr;
  });

  if (it != synths.end()) {
    return dynamic_cast<T*>((*it).get());
  } else {
    return nullptr;
  }
}

Synth* YmmyProcessor::synthTypeToSynth(SynthType synthType) {
  switch (synthType) {
    case FluidSynth:
      return findSynth<FluidSynthSynth>(synths);
    case YM2151:
      return findSynth<YM2151Synth>(synths);
  }
  return nullptr;
}

void YmmyProcessor::setSynthForChannel(int channelGroup, int channel, SynthType synthType) {
  auto synth = synthTypeToSynth(synthType);
  channelToSynthMap[(channelGroup*16) + channel] = synth;
}

