#include <iterator>
#include "fluidsynth.h"
#include "FluidSynthSynth.h"
#include "../../MidiConstants.h"

constexpr int NUM_CHANNELS = 48;

#ifdef _WIN32
#define FLUID_PRIi64 "I64d"
#else
#define FLUID_PRIi64 "lld"
#endif

fluid_long_long_t fileOffset;
fluid_long_long_t fileSize;

namespace FluidSynthParam {
  const juce::String FILTER_RESONANCE = "fs.filterResonance";
  const juce::String FILTER_CUT_OFF = "fs.filterCutOff";
  const juce::String ATTACK_RATE = "fs.attackRate";
  const juce::String HOLD = "fs.hold";
  const juce::String DECAY_RATE = "fs.decayRate";
  const juce::String SUSTAIN_LEVEL = "fs.sustainLevel";
  const juce::String RELEASE_RATE = "fs.releaseRate";

  IntParameter parameters[] = {
      {"fs.bank", "selected bank", 0, 128, 0},
      {"fs.preset", "selected preset", 0, 127, 0},
      {FluidSynthParam::ATTACK_RATE, "Attack Time", -12000, 8000, -12000},
      {FluidSynthParam::HOLD, "Hold Time", -12000, 5000, -12000},
      {FluidSynthParam::DECAY_RATE, "Decay Time", -12000, 8000, -12000},
      {FluidSynthParam::SUSTAIN_LEVEL, "Sustain Level", 0, 1440, 0},
      {FluidSynthParam::RELEASE_RATE, "Release Rate", -12000, 8000, -12000},
  };

  static std::map<juce::String, IntParameter> createParamIdMap() {
    std::map<juce::String, IntParameter> paramIdMap;
    for (const auto& param : parameters) {
      paramIdMap[param.id] = param;
    }
    return paramIdMap;
  }

  std::map<juce::String, IntParameter> paramIdToParam = createParamIdMap();
}


std::map<fluid_gen_type, juce::String> generatorToParam = {
//    {VOLUME_MSB, FluidSynthParam::MIDI_VOLUME},
    {GEN_FILTERFC, FluidSynthParam::FILTER_CUT_OFF},
    {GEN_FILTERQ, FluidSynthParam::FILTER_RESONANCE},
    {GEN_VOLENVATTACK, FluidSynthParam::ATTACK_RATE},
    {GEN_VOLENVHOLD, FluidSynthParam::HOLD},
    {GEN_VOLENVDECAY, FluidSynthParam::DECAY_RATE},
    {GEN_VOLENVSUSTAIN, FluidSynthParam::SUSTAIN_LEVEL},
    {GEN_VOLENVRELEASE, FluidSynthParam::RELEASE_RATE}
};

static std::map<juce::String, fluid_gen_type> createInverseMap() {
  std::map<juce::String, fluid_gen_type> inverseMap;
  for (const auto& pair : generatorToParam) {
    inverseMap[pair.second] = pair.first;
  }
  return inverseMap;
}

std::map<juce::String, fluid_gen_type> paramToGenerator = createInverseMap();



void *mem_open(const char *path) {
  void *buffer;

  if(path[0] != '&') {
    return nullptr;
  }
  sscanf(path, "&%p %" FLUID_PRIi64, &buffer, &fileSize);
  fileOffset = 0;
  return buffer;
}

int mem_read(void *buf, fluid_long_long_t count, void * data) {
  memcpy(buf, ((uint8_t*)data) + fileOffset, (size_t)count);
  fileOffset += count;
  return FLUID_OK;
}

int mem_seek(void * data, fluid_long_long_t offset, int whence) {
  if (whence == SEEK_SET) {
    fileOffset = offset;
  } else if (whence == SEEK_CUR) {
    fileOffset += offset;
  }  else if (whence == SEEK_END) {
    fileOffset = fileSize + offset;
  }
  return FLUID_OK;
}

int mem_close(void *handle) {
  fileOffset = 0;
  fileSize = 0;
  return FLUID_OK;
}

fluid_long_long_t mem_tell(void *handle) {
  return fileOffset;
}

FluidSynthSynth::FluidSynthSynth(AudioProcessorValueTreeState& valueTreeState)
    : Synth(valueTreeState), settings{nullptr, nullptr}, synth{nullptr, nullptr},
      channelGroup(0),
      currentSampleRate(44100), sfont_id(-1) {
  // initialize();

  vts.state.addListener(this);

  vts.addParameterListener("selectedGroup", this);
  vts.addParameterListener("selectedChannel", this);
  vts.addParameterListener("fs.bank", this);
  vts.addParameterListener("fs.preset", this);
  for (const auto &[param, genId]: paramToGenerator) {
    vts.addParameterListener(param, this);
  }
}

FluidSynthSynth::~FluidSynthSynth() {
  for (const auto &[param, genId]: paramToGenerator) {
    vts.removeParameterListener(param, this);
  }
  vts.removeParameterListener("fs.bank", this);
  vts.removeParameterListener("fs.preset", this);
  vts.removeParameterListener("selectedGroup", this);
  vts.removeParameterListener("selectedChannel", this);
  vts.state.removeListener(this);
}

std::unique_ptr<juce::AudioProcessorParameterGroup> FluidSynthSynth::createParameterGroup() {
  auto fluidSynthParams = std::make_unique<juce::AudioProcessorParameterGroup>("fluidsynth", "FluidSynth", "|");

  for (IntParameter p : FluidSynthParam::parameters) {
    fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(p.id, p.name, p.min, p.max, p.defaultVal));
  }
  return fluidSynthParams;
}

//void FluidSynthSynth::updateValueTreeState(AudioProcessorValueTreeState& vts) {
//  vts.state.appendChild({ "soundFont",
//                          {
//                            { "path", "" },
//                            { "memfile", var(nullptr, 0) }
//                          }, {} }, nullptr);
//  vts.state.appendChild({ "banks", {}, {} }, nullptr);
//
//  vts.state.appendChild({ "test",
//                         {
//                             { "fun", "" },
//                         }, {} }, nullptr);
//}

//const std::vector<ValueTree> FluidSynthSynth::getInitialChildValueTrees() {
const ValueTree FluidSynthSynth::getInitialChildValueTree() {
  return
    { "ignoredRoot", {},
      {
        { "soundFont",
         {
             { "path", "" },
             { "memfile", var(nullptr, 0) }
         }, {}
        },
        { "banks", {}, {} },
      }
    };
}

void FluidSynthSynth::receiveFile(juce::MemoryBlock& data, SynthFileType fileType) {
  if (fileType != SynthFileType::SoundFont2)
    return;
  unloadAndLoadFontFromMemory(data.getData(), data.getSize());
}

void FluidSynthSynth::handleSysex(MidiMessage& message) {
  auto sysexData = message.getSysExData();
  auto sysexDataSize = message.getSysExDataSize();

  if (sysexData[0] != 0x7D) {
    fluid_synth_sysex(
        synth.get(),
        reinterpret_cast<const char *>(message.getSysExData()),
        message.getSysExDataSize(),
        nullptr, // no response pointer because we have no interest in handling response currently
        nullptr, // no response_len pointer because we have no interest in handling response currently
        nullptr, // no handled pointer because we have no interest in handling response currently
        static_cast<int>(false));
    // return;
  }
  uint8_t sysexCommand = sysexData[1];
  if (sysexCommand >= 0 && sysexCommand < 16) {
    channelGroup = static_cast<int>(sysexData[1]);
    auto wrappedMsg = juce::MidiMessage(sysexData + 2, message.getSysExDataSize() - 2, 0);
    processMidiMessage(wrappedMsg);
    return;
  }
  switch (sysexCommand) {
    // case 0x10: {  // SF2 send start {
    //   if (sysexData[2] != 0x00) {
    //     DBG("FluidSynthSynth received send File start command for wrong file type: " + sysexData[2]);
    //     break;
    //   }
    //   int bytesRead = 0;
    //   incomingSF2Size = read6BitVariableLengthQuantity(sysexData+3, sysexDataSize-3, bytesRead);
    //   incomingSF2File.reset();
    //   break;
    // }
    // case 0x11: {
    //   if (sysexData[2] != 0x00) {
    //     DBG("FluidSynthSynth received send File data command for wrong file type: " + sysexData[2]);
    //     break;
    //   }
    //   if (incomingSF2Size == 0) {
    //     break;
    //   }
    //   auto destBuf = new uint8_t[static_cast<unsigned long>(sysexDataSize + 8)];
    //   int inOffset = 3;     // The first three sysex bytes are manufacturer ID and command, and file type, so skip past them
    //   int outOffset = 0;
    //   while (inOffset < sysexDataSize) {
    //     read7BitChunk(sysexData+inOffset, destBuf+outOffset);
    //     inOffset += 8;
    //     outOffset += 7;
    //   }
    //   incomingSF2Size -= outOffset;
    //   if (incomingSF2Size <= 0) {
    //     // shave off any extraneous bytes that weren't read
    //     outOffset += incomingSF2Size;
    //     incomingSF2Size = 0;
    //   }
    //   incomingSF2File.append(destBuf, static_cast<size_t>(outOffset));
    //   // We should probably load the SF2 if incomingSF2Size == 0
    //
    //   if (incomingSF2Size == 0) {
    //     unloadAndLoadFontFromMemory(incomingSF2File.getData(), incomingSF2File.getSize());
    //   }
    //
    //   delete[] destBuf;
    //   break;
    // }
    case 0x7F:
      fluid_synth_system_reset(synth.get());
      break;
    default:
      break;
  }
}

void FluidSynthSynth::processMidiMessage(MidiMessage& m) {
  auto rawData = m.getRawData();

  if (rawData[0] == 0xF0) {
    handleSysex(m);
    return;
  }

  if (rawData[0] >= 0xF1) {
    return;
  }

  const uint8_t eventNibble = rawData[0] & 0xf0;
  int channelGroupOffset = channelGroup * 16;

  switch (eventNibble) {
    case NOTE_OFF:
      fluid_synth_noteoff(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getNoteNumber());
      break;
    case NOTE_ON:
      fluid_synth_noteon(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getNoteNumber(), m.getVelocity());
//      fluid_synth_program_change(synth.get(), m.getChannel() - 1 + channelGroupOffset, 31);
      break;
    case KEY_PRESSURE:
      fluid_synth_channel_pressure(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getChannelPressureValue());
      break;
    case CONTROL_CHANGE:
      if (m.getControllerNumber() == 115) {
        int value = m.getControllerValue();
        uint8_t programNum = value & 0x7F;
        uint8_t bank = (value >> 7) & 0x7F;
        fluid_synth_set_channel_type(synth.get(), m.getChannel() - 1 + channelGroupOffset, CHANNEL_TYPE_MELODIC);
        fluid_synth_program_change(synth.get(), m.getChannel() - 1 + channelGroupOffset, programNum);
      } else {
        fluid_synth_cc(synth.get(), m.getChannel() - 1 + channelGroupOffset,
                       m.getControllerNumber(), m.getControllerValue());
      }
      break;
    case PROGRAM_CHANGE: {
      fluid_synth_set_channel_type(synth.get(), m.getChannel() - 1 + channelGroupOffset, CHANNEL_TYPE_MELODIC);
      int result = fluid_synth_program_change(synth.get(), m.getChannel() - 1 + channelGroupOffset,
                                              m.getProgramChangeNumber());
      if (result == FLUID_OK) {

      }
      break;
    }
    case CHANNEL_PRESSURE:
      fluid_synth_channel_pressure(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getChannelPressureValue());
      break;
    case PITCH_BEND:
      fluid_synth_pitch_bend(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getPitchWheelValue());
      break;
    case MIDI_SYSEX:
      int channelGroupMaybe = CheckForChannelGroupSysExEvent(m);
      if (channelGroupMaybe != -1) {
        channelGroup = channelGroupMaybe;
        return;
      }
      fluid_synth_sysex(synth.get(), reinterpret_cast<const char *>(m.getSysExData()), m.getSysExDataSize(),
          nullptr, nullptr, nullptr, static_cast<int>(false));
      break;
  }
}

void FluidSynthSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  MidiBuffer processedMidi;
  int time;
  MidiMessage m;

  for (MidiBuffer::Iterator i{midiMessages}; i.getNextEvent(m, time);) {
    processMidiMessage(m);
  }

  buffer.clear();
  auto bufferWritePointers = buffer.getArrayOfWritePointers();
  if (buffer.getNumChannels() < 2) {
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,"Error",
                                     "Expected at least 2 write buffers for stereo playback. Is there a problem with the current audio device?.");
    return;
  }
  float* writeBuffers[2] = {bufferWritePointers[0], bufferWritePointers[1] };

  fluid_synth_process(synth.get(), buffer.getNumSamples(), 2, writeBuffers, 2, writeBuffers);
}

void FluidSynthSynth::prepareToPlay(double sampleRate, int samplesPerBlock) {

  double sampleRateSetting;
  fluid_settings_getnum(settings.get(), "synth.sample-rate", &sampleRateSetting);
  if (!synth.get() || sampleRateSetting != sampleRate) {

    settings = { new_fluid_settings(), delete_fluid_settings };

    fluid_settings_setnum(settings.get(), "synth.sample-rate", sampleRate);
    fluid_settings_setstr(this->settings.get(), "synth.midi-bank-select", "gs");
    fluid_settings_setint(this->settings.get(), "synth.midi-channels", NUM_CHANNELS);
#if JUCE_DEBUG
    fluid_settings_setint(settings.get(), "synth.verbose", 1);
#endif

    synth = { new_fluid_synth(settings.get()), delete_fluid_synth };

    fluid_synth_set_reverb_on(synth.get(), true);
    fluid_synth_set_reverb(synth.get(), 0.7, 0.4, 0.5, 0.5);

    fluid_sfloader_t *my_sfloader = new_fluid_defsfloader(settings.get());
    fluid_sfloader_set_callbacks(my_sfloader, mem_open, mem_read, mem_seek, mem_tell, mem_close);
    fluid_synth_add_sfloader(synth.get(), my_sfloader);

    reset();
  }
}

int FluidSynthSynth::getNumPrograms() {
  return 128;
}

int FluidSynthSynth::getCurrentProgram() {
  RangedAudioParameter *param{vts.getParameter("fs.preset")};
  jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
  AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
  return castParam->get();
}

const juce::String FluidSynthSynth::getProgramName (int index) {
  fluid_sfont_t* sfont{
      sfont_id == -1
          ? nullptr
          : fluid_synth_get_sfont_by_id(synth.get(), sfont_id)
  };
  if (!sfont) {
    String presetName{"Preset "};
    return presetName << index;
  }
  RangedAudioParameter *param{vts.getParameter("fs.bank")};
  jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
  AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
  int bank{castParam->get()};

  fluid_preset_t *preset{fluid_sfont_get_preset(
      sfont,
      bank,
      index)};
  if (!preset) {
    String presetName{"Preset "};
    return presetName << index;
  }
  return {fluid_preset_get_name(preset)};
}

void FluidSynthSynth::setCurrentProgram (int index) {
  RangedAudioParameter *param{vts.getParameter("fs.preset")};
  jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
  AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
  // setCurrentProgram() gets invoked from non-message thread.
  // AudioParameterInt#operator= will activate any listeners of audio parameter "preset".
  // This includes TableComponent, who will update its UI.
  // we need to lock the message thread whilst it does that UI update.
  const MessageManagerLock mmLock;
  *castParam = index;
}

const StringArray FluidSynthSynth::programChangeParams{"fs.bank", "fs.preset"};
void FluidSynthSynth::parameterChanged(const String& parameterID, float newValue) {
  DBG("FluidSynthSynth::parameterChanged() " + parameterID);
  if (programChangeParams.contains(parameterID)) {
    int bank, preset;
    {
      RangedAudioParameter *param{vts.getParameter("fs.bank")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      bank = castParam->get();
    }
    {
      RangedAudioParameter *param{vts.getParameter("fs.preset")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      preset = castParam->get();
    }
    int bankOffset = fluid_synth_get_bank_offset(synth.get(), sfont_id);
    fluid_synth_program_select(
      synth.get(),
      selectedChannel + (selectedGroup*16),
      sfont_id,
      static_cast<unsigned int>(bankOffset + bank),
      static_cast<unsigned int>(preset));

    // We should probably reset the nrpn overrides when a new program is selected
//    updateParamsFromSynth();
  } else if (
      // https://stackoverflow.com/a/55482091/5257399
      auto it{paramToGenerator.find(parameterID)};
      it != end(paramToGenerator)
    ) {
    RangedAudioParameter *param{vts.getParameter(parameterID)};
    jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
    AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
    int value{castParam->get()};
//    int controllerNumber{static_cast<int>(it->second)};
    int generatorNumber{static_cast<int>(it->second)};

    printf("PARAMETER CHANGED: %d\n", generatorNumber);
    fluid_synth_set_gen_override(synth.get(), selectedChannel + (selectedGroup*16), generatorNumber, static_cast<float>(value));
//    fluid_voice_gen_set(synth.get(), controllerNumber, static_cast<float>(value));

//    fluid_synth_cc(synth.get(), selectedChannel, NRPN_MSB, 120);
//    fluid_synth_cc(synth.get(), selectedChannel, NRPN_LSB, generatorNumber);
//    value += 8192;
//    fluid_synth_cc(synth.get(), selectedChannel, DATA_ENTRY_LSB, value & 0x7F);
//    fluid_synth_cc(synth.get(), selectedChannel, DATA_ENTRY_MSB, (value>>7) & 0x7F);

//    fluid_synth_cc(synth.get(), selectedChannel, controllerNumber+32, value);
//    fluid_synth_cc(synth.get(), selectedChannel, controllerNumber, value);
  }
}

void FluidSynthSynth::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                               const Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() == StringRef("soundFont")) {
    if (property == StringRef("path")) {
      String soundFontPath = treeWhosePropertyHasChanged.getProperty("path", "");
      if (soundFontPath.isNotEmpty()) {
        unloadAndLoadFont(soundFontPath);
      }
    } else if (property == StringRef("memfile")) {
      var memfile = treeWhosePropertyHasChanged.getProperty("memfile", var(nullptr, 0));

      if (memfile.isBinaryData() && memfile.getBinaryData()->getSize() != 0) {
        const size_t dataSize = memfile.getBinaryData()->getSize();
        unloadAndLoadFontFromMemory(memfile.getBinaryData()->getData(), dataSize);
      }
    }
  } else if (treeWhosePropertyHasChanged.getType() == StringRef("settings")) {
    if (property == StringRef("selectedChannel") || property == StringRef("selectedGroup")) {
      selectedGroup = treeWhosePropertyHasChanged.getProperty("selectedGroup", 0);
      selectedChannel = treeWhosePropertyHasChanged.getProperty("selectedChannel", 0);

      updateParamsFromSynth();
//      for (const auto &[param, genId]: paramToGenerator) {
//        auto p = vts.getParameter(param);
//        if (!p) {
//          continue;
//        }
//        auto paramVal = fluid_synth_get_gen(synth.get(), selectedChannel + (selectedGroup*16), genId);
//        printf("genID: %d: %f\n", genId, paramVal);
//        auto range = p->getNormalisableRange();
//        p->setValueNotifyingHost(range.convertTo0to1(paramVal));
//      }
    }
  }
}

void FluidSynthSynth::updateParamsFromSynth() {
  for (const auto &[param, genId]: paramToGenerator) {
    auto p = vts.getParameter(param);
    if (!p) {
      continue;
    }
    auto paramVal = fluid_synth_get_gen(synth.get(), selectedChannel + (selectedGroup*16), genId);
    printf("genID: %d: %f\n", genId, paramVal);
    auto range = p->getNormalisableRange();
    p->setValueNotifyingHost(range.convertTo0to1(paramVal));
  }
}

void FluidSynthSynth::setControllerValue(int controller, int value) {
    fluid_synth_cc(synth.get(), selectedChannel + (selectedGroup*16), controller, value);
}

void FluidSynthSynth::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {
//  vts.state.removeListener(this);
//  vts.state.addListener(this);

  vts.addParameterListener("fs.bank", this);
  vts.addParameterListener("fs.preset", this);
  for (const auto &[param, genId]: paramToGenerator) {
    vts.addParameterListener(param, this);
  }

  vts.state.addListener(this);

}

void FluidSynthSynth::unloadAndLoadFont(const String &absPath) {
  // in the base case, there is no font loaded
  if (fluid_synth_sfcount(synth.get()) > 0) {
    // if -1 is returned, that indicates failure
    // not really sure how to handle "fail to unload"
    fluid_synth_sfunload(synth.get(), sfont_id, 1);
    sfont_id = -1;
  }
  loadFont(absPath);
}

void FluidSynthSynth::unloadAndLoadFontFromMemory(void *sf, size_t fileSize) {
  // in the base case, there is no font loaded
  if (fluid_synth_sfcount(synth.get()) > 0) {
    // if -1 is returned, that indicates failure
    // not really sure how to handle "fail to unload"
    fluid_synth_sfunload(synth.get(), sfont_id, 1);
    sfont_id = -1;
  }
  loadFontFromMemory(sf, fileSize);
}

void FluidSynthSynth::loadFont(const String &absPath) {
  if (!absPath.isEmpty()) {
    sfont_id = fluid_synth_sfload(synth.get(), absPath.toStdString().c_str(), 1);
    // if -1 is returned, that indicates failure
  }
  // refresh regardless of success, if only to clear the table
  refreshBanks();
}

void FluidSynthSynth::loadFontFromMemory(void *sf, fluid_long_long_t fileSize) {
  char abused_filename[128];
  snprintf(abused_filename, 128, "&%p %" FLUID_PRIi64, sf, fileSize);

  sfont_id = fluid_synth_sfload(synth.get(), abused_filename, 1);
  // if -1 is returned, that indicates failure
  // refresh regardless of success, if only to clear the table
  refreshBanks();
}

void FluidSynthSynth::reset() {
  fluid_synth_system_reset(synth.get());
}


void FluidSynthSynth::refreshBanks() {
  // ValueTree banks = ValueTree("banks");
  ValueTree banks = vts.state.getOrCreateChildWithName("banks", nullptr); // Access the existing tree or create it

  banks.removeAllChildren(nullptr); // Clear any previous banks if needed

  if (sfont_id == -1) {
    return;
  }
  fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(synth.get(), sfont_id);
  if (!sfont) {
    return;
  }

  int greatestEncounteredBank = -1;
  ValueTree bank;

  fluid_sfont_iteration_start(sfont);
  for(fluid_preset_t* preset = fluid_sfont_iteration_next(sfont);
       preset != nullptr;
       preset = fluid_sfont_iteration_next(sfont)) {
    int bankNum = fluid_preset_get_banknum(preset);
    if (bankNum > greatestEncounteredBank) {
      if (greatestEncounteredBank > -1) {
        banks.appendChild(bank, nullptr);
      }
      bank = { "fs.bank", { { "num", bankNum } } };
      greatestEncounteredBank = bankNum;
    }
    bank.appendChild({ "fs.preset", {
        { "num", fluid_preset_get_num(preset) },
        { "name", String(fluid_preset_get_name(preset)) }
    }, {} }, nullptr);
  }
  if (greatestEncounteredBank > -1) {
    banks.appendChild(bank, nullptr);
  }
//  vts.state.getChildWithName("banks").copyPropertiesAndChildrenFrom(banks, nullptr);
//  vts.state.getChildWithName("banks").sendPropertyChangeMessage("synthetic");

#if JUCE_DEBUG
//    unique_ptr<XmlElement> xml{valueTreeState.state.createXml()};
//    Logger::outputDebugString(xml->createDocument("",false,false));
#endif
}