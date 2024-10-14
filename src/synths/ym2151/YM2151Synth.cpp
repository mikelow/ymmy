#include "YM2151Synth.h"
#include "../../YmmyProcessor.h"
#include "../../MidiConstants.h"
#include <random>

// The clock rate used by CPS1. We'll need to make this dynamic when we support systems that run at 4mhz.
constexpr int CPS1_YM2151_CLOCK_RATE = 3579545;

uint8_t ascendingKeyScaleTable[] = {
  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x5,
  0xA,  0xF, 0x14, 0x19,
  0x1E, 0x22, 0x27, 0x2C,
  0x31, 0x36, 0x3B, 0x40,
  0x45, 0x4A, 0x4F, 0x54,
  0x59, 0x5E, 0x62, 0x67,
  0x6C, 0x71, 0x76, 0x7B,
  0x80, 0x85, 0x8A, 0x8F,
  0x94, 0x99, 0x9E, 0xA2,
  0xA7, 0xAC, 0xB1, 0xB6,
  0xBB, 0xC0, 0xC5, 0xCA,
  0xCF, 0xD4, 0xD9, 0xDE,
  0xE2, 0xE7, 0xEC, 0xF1,
  0xF6, 0xFB, 0xFE, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t descendingKeyScaleTable[] = {
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFE, 0xFD, 0xFC, 0xFA,
  0xF8, 0xF6, 0xF3, 0xF0,
  0xEC, 0xE8, 0xE4, 0xDF,
  0xDA, 0xD4, 0xCF, 0xC8,
  0xC2, 0xBB, 0xB3, 0xAB,
  0xA3, 0x9B, 0x92, 0x89,
  0x7F, 0x75, 0x6C, 0x63,
  0x5B, 0x53, 0x4B, 0x43,
  0x3C, 0x36, 0x2F, 0x2A,
  0x24, 0x1F, 0x1A, 0x16,
  0x12,  0xE,  0xB,  0x8,
  0x6,  0x4,  0x2,  0x1,
  0x1,  0x1,  0x1,  0x0,
  0x0,  0x0,  0x0,  0x0
};

uint8_t calculateKeyScaleAttenuation(char key_scale_sensitivity, uint8_t note) {
  uint8_t index = (note >> 1) & 0x3F;
  uint8_t key_scale_factor = (key_scale_sensitivity >= 0) ?
                                ascendingKeyScaleTable[index] :
                                descendingKeyScaleTable[index];
  uint8_t ksa = key_scale_sensitivity & 0xF; // Use lower 4 bits (magnitude)
  uint16_t product = key_scale_factor * ksa;
  return (uint8_t)(product >> 7); // Equivalent to dividing by 128
}


namespace YM2151SynthParam {

  IntParameter parameters[] = {
      {"ym2151.bank", "selected bank", 0, 128, 0},
      {"ym2151.preset", "selected preset", 0, 127, 0},
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


YM2151Synth::YM2151Synth(YmmyProcessor* p, AudioProcessorValueTreeState& valueTreeState)
    : Synth(valueTreeState),
      processor(p),
      channelGroup(0) {
//  initialize();

  vts.state.addListener(this);

  vts.addParameterListener("selectedGroup", this);
  vts.addParameterListener("selectedChannel", this);
  vts.addParameterListener("ym2151.bank", this);
  vts.addParameterListener("ym2151.preset", this);
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.addParameterListener(param, this);
//  }
}

YM2151Synth::~YM2151Synth() {
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.removeParameterListener(param, this);
//  }
  vts.removeParameterListener("ym2151.bank", this);
  vts.removeParameterListener("ym2151.preset", this);
  vts.removeParameterListener("selectedGroup", this);
  vts.removeParameterListener("selectedChannel", this);
  vts.state.removeListener(this);
}

std::unique_ptr<juce::AudioProcessorParameterGroup> YM2151Synth::createParameterGroup() {
  auto ym2151Params = std::make_unique<juce::AudioProcessorParameterGroup>("ym2151", "YM2151", "|");

  for (IntParameter p : YM2151SynthParam::parameters) {
    ym2151Params->addChild(std::make_unique<juce::AudioParameterInt>(p.id, p.name, p.min, p.max, p.defaultVal));
  }
  return ym2151Params;
}

const ValueTree YM2151Synth::getInitialChildValueTree() {
  return
    { "ignoredRoot", {},
      {
        { "opmFile",
          {
            { "path", "" },
          }, {}
        },
        { "ym2151.banks", {}, {} },
      },
    };
}

void YM2151Synth::receiveFile(juce::MemoryBlock& data, SynthFileType fileType) {
  if (fileType != SynthFileType::YM2151_OPM)
    return;

  auto patches = OPMFileLoader::parseOpmString(data.toString().toStdString());
  refreshBanks(patches);
}

void YM2151Synth::changePreset(OPMPatch& patch, int channel, bool enableLFO) {
  midiChannelState[channel].CON = patch.channelParams.CON;
  midiChannelState[channel].SLOT_MASK = patch.channelParams.SLOT_MASK;
  midiChannelState[channel].cpsParams = patch.cpsParams;
  for (int i = 0; i < 4; ++i) {
    midiChannelState[channel].TL[i] = patch.opParams[i].TL;
  }
  interface.changePreset(patch, channel, enableLFO);
}

void YM2151Synth::handleSysex(MidiMessage& message) {
  auto sysexData = message.getSysExData();
  auto sysexDataSize = message.getSysExDataSize();

  // if (sysexData[0] == 0x7F && sysexDataSize >= 6) {
  //   if (sysexData[2] == 0x04 && sysexData[3] == 0x01) {
  //     // Master volume
  //     uint8_t masterVolumeLSB = sysexData[4];
  //     uint8_t masterVolumeMSB = sysexData[5];
  //   }
  //   return;
  // }
  if (sysexData[0] != 0x7D) {
    return;
  }
  uint8_t sysexCommand = sysexData[1];
  if (sysexCommand >= 0 && sysexCommand < 16) {
    channelGroup = static_cast<int>(sysexData[1]);
    auto wrappedMsg = juce::MidiMessage(sysexData + 2, message.getSysExDataSize() - 2, 0);
    processMidiMessage(wrappedMsg);
    return;
  }
  switch (sysexCommand) {
    case 0x7F:
      // fluid_synth_system_reset(synth.get());
        break;
    default:
      break;
  }
}

void YM2151Synth::processMidiMessage(MidiMessage& m) {
  auto rawData = m.getRawData();
  if (rawData[0] == 0xF0) {
    handleSysex(m);
    return;
  }

  const uint8_t eventNibble = rawData[0] & 0xf0;
  int channelGroupOffset = channelGroup * 16;
  int channel = (m.getChannel() - 1 + channelGroupOffset) % 8;
  YM2151MidiChannelState& chanState = midiChannelState[channel];

  switch (eventNibble) {
    case NOTE_OFF:
      interface.write(8, channel);
      break;
    case NOTE_ON: {
      int absNote = m.getNoteNumber();
      int vel = m.getVelocity();

      // temporary adjustment to get notes aligned with 3.58mhz freq
      absNote -= 13;
      if ((chanState.KF & 0x80) > 0) {
        absNote -= 1;
      }
      int octave = absNote / 12;
      int note = absNote % 12;

      const int noteTable[12] = {0,1,2,4,5,6,8,9,10,12,13,14};

      uint8_t finalNoteValue = ((octave & 7) << 4) | noteTable[note];

      // Set volume of note
      uint8_t channelVolume = chanState.volume;
      uint8_t channelVolumeAtten = 0x7F - channelVolume;
      channelVolumeAtten += 0x7F - vel;

      uint8_t attenByte;
      uint8_t volKeyScaleAtten;
      uint8_t CON_limits[4] = { 7, 5, 4, 0 };
      for (int i = 0; i < 4; i++) {
        uint8_t keyScale = chanState.cpsParams.vol_data[i].key_scale_sensitivity;
        volKeyScaleAtten = calculateKeyScaleAttenuation(keyScale, finalNoteValue);
        auto conLimit = CON_limits[i];
        uint32_t finalAttenuation = volKeyScaleAtten;
        if (chanState.CON < conLimit) {
          finalAttenuation += chanState.TL[i];
        } else {
          finalAttenuation += chanState.TL[i] + channelVolumeAtten;
        }
        attenByte = static_cast<uint8_t>(std::min(finalAttenuation, 0x7FU));
        interface.write(0x60 + (i*8) + channel, attenByte);
      }

      printf("OCT: %d    NOTE: %d  val: %X  atten: %X  ksAtten: %X \n", octave, note, finalNoteValue, attenByte, volKeyScaleAtten);
      // Send octave / note
      interface.write(0x28 + channel, finalNoteValue);
      // Send note on
      interface.write(8, chanState.SLOT_MASK | channel);

      // If the instrument reset_lfo flag is set, we reset LFO phase after note on
      if (chanState.cpsParams.reset_lfo) {
        interface.write(1, 2);
        interface.write(1, 0);
      }
      break;
    }
    case KEY_PRESSURE:
      break;
    case CONTROL_CHANGE:
      switch (m.getControllerNumber()) {
        case VOLUME_MSB: {
          int value = m.getControllerValue();
          midiChannelState[channel].volume = value;
          break;
        }
      }
      break;
    case PROGRAM_CHANGE: {
      auto patch = loadPresetFromVST(0, m.getProgramChangeNumber());
      changePreset(patch, channel, patch.cpsParams.enable_lfo);
      break;
    }
    case PITCH_BEND: {
      // first convert to cents by assuming 2 semitone range (for now)
      int bend = m.getPitchWheelValue();
      double cents = ((bend - 8192.0) / 8192.0) * 200;
      if (cents < 0) {
        cents = 100.0 + cents;
      }

      // next convert to a OPM Key Fraction value
      uint8_t kf = static_cast<uint8_t>(std::round(cents / 1.587301587301587));
      midiChannelState[channel].KF = kf << 2;
      constexpr uint8_t minKF = 0;
      constexpr uint8_t maxKF = 63;
      kf = std::clamp(kf, minKF, maxKF);
      interface.write(0x30 + channel, kf << 2);
      break;
    }
    case CHANNEL_PRESSURE:
      break;

    case MIDI_SYSEX:
      break;
  }
}

void YM2151Synth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  MidiBuffer processedMidi;
  int time;
  MidiMessage m;

  for (MidiBuffer::Iterator i{midiMessages}; i.getNextEvent(m, time);) {
    processMidiMessage(m);
  }

  auto bufferWritePointers = buffer.getArrayOfWritePointers();
  if (buffer.getNumChannels() < 2) {
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,"Error",
                                     "Expected at least 2 write buffers for stereo playback. Is there a problem with the current audio device?.");
    return;
  }

  auto nativeBufferWritePointers = nativeBuffer->getArrayOfWritePointers();
  float* writeBuffers[2] = {nativeBufferWritePointers[0], nativeBufferWritePointers[1]};

  int numNativeSamples = nativeBuffer->getNumSamples();
  double ratio = static_cast<double>(nativeBuffer->getNumSamples()) / processor->getSampleRate();
  interface.generate(writeBuffers, ceil(buffer.getNumSamples() * ratio));

  // Mix generated samples with existing content in the buffer
  for (int channel = 0; channel < 2; ++channel) {
    float* output = bufferWritePointers[channel];
    // resamplers[channel].process(ratio, nativeBufferWritePointers[channel], output, buffer.getNumSamples());
    float tempBuffer[buffer.getNumSamples()];

    // Use the resampler to write into a temporary buffer
    resamplers[channel].process(ratio, nativeBufferWritePointers[channel], tempBuffer, buffer.getNumSamples());

    // Add the contents of the tempBuffer to the output buffer
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      output[sample] += tempBuffer[sample];
    }
  }
}

void YM2151Synth::prepareToPlay(double sampleRate, int samplesPerBlock) {
  for (int i = 0; i < 2; ++i) {
    resamplers[i].reset();
  }

  uint32_t numSamples = interface.sample_rate(CPS1_YM2151_CLOCK_RATE);
//  uint32_t numSamples = interface.sample_rate(4000000);
  nativeBuffer = std::make_unique<AudioBuffer<float>>(2, numSamples);
  reset();
}

int YM2151Synth::getNumPrograms() {
  return 128;
}

int YM2151Synth::getCurrentProgram() {
  return 0;
}

void YM2151Synth::setCurrentProgram (int index) {
}

const juce::String YM2151Synth::getProgramName (int index) {
  return "Preset";
}

const StringArray YM2151Synth::programChangeParams{"ym2151.bank", "ym2151.preset"};
void YM2151Synth::parameterChanged(const String& parameterID, float newValue) {
  if (programChangeParams.contains(parameterID)) {
    int bank, preset;
    {
      RangedAudioParameter *param{vts.getParameter("ym2151.bank")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      bank = castParam->get();
    }
    {
      RangedAudioParameter *param{vts.getParameter("ym2151.preset")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      preset = castParam->get();
    }
    auto patch = loadPresetFromVST(bank, preset);
    // TODO: presumably translate the bank/channel to the YM2151 channel
    changePreset(patch, selectedChannel, patch.cpsParams.enable_lfo);
  }
}

OPMPatch YM2151Synth::loadPresetFromVST(int bankNum, int presetNum) {

  OPMPatch patch = OPMPatch();

  auto banks = vts.state.getChildWithName("ym2151.banks");
  int numBanks = banks.getNumChildren();

  if (bankNum > numBanks) {
    return patch;
  }

  auto bank = banks.getChild(bankNum);
  int numPresets = bank.getNumChildren();
  if (presetNum > numPresets) {
    return patch;
  }
  auto preset = bank.getChild(presetNum);

  patch.number = static_cast<uint8_t>(static_cast<int>(preset.getProperty("num")));
  patch.name = preset.getProperty("name").toString().toStdString();

  auto lfo = preset.getChildWithName("LFO");
  patch.lfoParams.LFRQ = static_cast<uint8_t>(static_cast<int>(lfo.getProperty("LFRQ")));
  patch.lfoParams.AMD = static_cast<uint8_t>(static_cast<int>(lfo.getProperty("AMD")));
  patch.lfoParams.PMD = static_cast<uint8_t>(static_cast<int>(lfo.getProperty("PMD")));
  patch.lfoParams.WF = static_cast<uint8_t>(static_cast<int>(lfo.getProperty("WF")));
  patch.lfoParams.NFRQ = static_cast<uint8_t>(static_cast<int>(lfo.getProperty("NFRQ")));

  auto ch = preset.getChildWithName("CH");
  patch.channelParams.PAN = static_cast<uint8_t>(static_cast<int>(ch.getProperty("PAN")));
  patch.channelParams.FL = static_cast<uint8_t>(static_cast<int>(ch.getProperty("FL")));
  patch.channelParams.CON = static_cast<uint8_t>(static_cast<int>(ch.getProperty("CON")));
  patch.channelParams.AMS = static_cast<uint8_t>(static_cast<int>(ch.getProperty("AMS")));
  patch.channelParams.PMS = static_cast<uint8_t>(static_cast<int>(ch.getProperty("PMS")));
  patch.channelParams.SLOT_MASK = static_cast<uint8_t>(static_cast<int>(ch.getProperty("SLOT_MASK")));
  patch.channelParams.NE = static_cast<uint8_t>(static_cast<int>(ch.getProperty("NE")));


  for (size_t i = 0; i < sizeof(patch.opParams) / sizeof(patch.opParams[0]); ++i) {
    String opName = String("OP" + std::to_string(i));
    auto op = preset.getChildWithName(opName);
    patch.opParams[i].AR = static_cast<uint8_t>(static_cast<int>(op.getProperty("AR")));
    patch.opParams[i].D1R = static_cast<uint8_t>(static_cast<int>(op.getProperty("D1R")));
    patch.opParams[i].D2R = static_cast<uint8_t>(static_cast<int>(op.getProperty("D2R")));
    patch.opParams[i].RR = static_cast<uint8_t>(static_cast<int>(op.getProperty("RR")));
    patch.opParams[i].D1L = static_cast<uint8_t>(static_cast<int>(op.getProperty("D1L")));
    patch.opParams[i].TL = static_cast<uint8_t>(static_cast<int>(op.getProperty("TL")));
    patch.opParams[i].KS = static_cast<uint8_t>(static_cast<int>(op.getProperty("KS")));
    patch.opParams[i].MUL = static_cast<uint8_t>(static_cast<int>(op.getProperty("MUL")));
    patch.opParams[i].DT1 = static_cast<uint8_t>(static_cast<int>(op.getProperty("DT1")));
    patch.opParams[i].DT2 = static_cast<uint8_t>(static_cast<int>(op.getProperty("DT2")));
    patch.opParams[i].AMS_EN = static_cast<uint8_t>(static_cast<int>(op.getProperty("AMS-EN")));
  }

  auto cps = preset.getChildWithName("CPS");
  patch.cpsParams.enable_lfo = static_cast<bool>(static_cast<int>(cps.getProperty("enable_lfo")));
  patch.cpsParams.reset_lfo = static_cast<bool>(static_cast<int>(cps.getProperty("reset_lfo")));

  for (size_t i = 0; i < sizeof(patch.cpsParams.vol_data) / sizeof(patch.cpsParams.vol_data[0]); ++i) {
    String opName = String("CPS_OP" + std::to_string(i));
    auto op = preset.getChildWithName(opName);
    patch.cpsParams.vol_data[i].key_scale_sensitivity = static_cast<uint8_t>(static_cast<int>(op.getProperty("vol_key_scale")));
    patch.cpsParams.vol_data[i].extra_atten = static_cast<uint8_t>(static_cast<int>(op.getProperty("extra_atten")));
  }

  return patch;
}


void YM2151Synth::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                           const Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() == StringRef("opmFile")) {
    if (property == StringRef("path")) {
      String opmPath = treeWhosePropertyHasChanged.getProperty("path", "");
      if (opmPath.isNotEmpty()) {
        auto patches = OPMFileLoader::parseOpmFile(opmPath.toStdString());
        refreshBanks(patches);
      }
    }
  } else if (treeWhosePropertyHasChanged.getType() == StringRef("settings")) {
    if (property == StringRef("selectedChannel") || property == StringRef("selectedGroup")) {
      selectedGroup = treeWhosePropertyHasChanged.getProperty("selectedGroup", 0);
      selectedChannel = treeWhosePropertyHasChanged.getProperty("selectedChannel", 0);
    }
  }
}

void YM2151Synth::setControllerValue(int controller, int value) {
}

void YM2151Synth::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {
  //  vts.state.removeListener(this);
  //  vts.state.addListener(this);

  vts.addParameterListener("ym2151.bank", this);
  vts.addParameterListener("ym2151.preset", this);
  //  for (const auto &[param, genId]: paramToGenerator) {
  //    vts.addParameterListener(param, this);
  //  }

  vts.state.addListener(this);
}

void YM2151Synth::reset() {
  interface.resetGlobal();
  for (int i = 0; i < 8; i++) {
    interface.resetChannel(i);
    midiChannelState[i] = {};
  }
}

void YM2151Synth::refreshBanks(std::vector<OPMPatch>& patches) {
  ValueTree banks = vts.state.getOrCreateChildWithName("ym2151.banks", nullptr); // Access the existing tree or create it

  banks.removeAllChildren(nullptr); // Clear any previous banks if needed

  ValueTree bank;

  bank = { "bank", { { "num", 0 }}};
  for (auto patch : patches) {
    ValueTree preset = { "preset", {
        { "num", patch.number },
        { "name", String(patch.name) },
      }, {
        { "LFO", {
            { "LFRQ", patch.lfoParams.LFRQ },
            { "AMD", patch.lfoParams.AMD },
            { "PMD", patch.lfoParams.PMD },
            { "WF", patch.lfoParams.WF },
            { "NFRQ", patch.lfoParams.NFRQ },
          }, {},
        },
        { "CH", {
            { "PAN", patch.channelParams.PAN },
            { "FL", patch.channelParams.FL },
            { "CON", patch.channelParams.CON },
            { "AMS", patch.channelParams.AMS },
            { "PMS", patch.channelParams.PMS },
            { "SLOT_MASK", patch.channelParams.SLOT_MASK},
            { "NE", patch.channelParams.NE },
          }, {},
        }
      }
    };
    for (int i = 0; i < sizeof(patch.opParams)/sizeof(patch.opParams[0]); i++) {
      String opName = "OP" + std::to_string(i);
      preset.appendChild({ opName, {
          { "AR", patch.opParams[i].AR },
          { "D1R", patch.opParams[i].D1R },
          { "D2R", patch.opParams[i].D2R },
          { "RR", patch.opParams[i].RR },
          { "D1L", patch.opParams[i].D1L },
          { "TL", patch.opParams[i].TL },
          { "KS", patch.opParams[i].KS },
          { "MUL", patch.opParams[i].MUL },
          { "DT1", patch.opParams[i].DT1 },
          { "DT2", patch.opParams[i].DT2 },
          { "AMS-EN", patch.opParams[i].AMS_EN },
         }, {},
      }, nullptr);
    }
    preset.appendChild({ "CPS", {
          { "enable_lfo", patch.cpsParams.enable_lfo },
          { "reset_lfo", patch.cpsParams.reset_lfo },
         }, {},
      }, nullptr);
    for (int i = 0; i < sizeof(patch.cpsParams.vol_data)/sizeof(patch.cpsParams.vol_data[0]); i++) {
      String opName = "CPS_OP" + std::to_string(i);
      preset.appendChild({ opName, {
          { "vol_key_scale", patch.cpsParams.vol_data[i].key_scale_sensitivity },
          { "extra_atten", patch.cpsParams.vol_data[i].extra_atten },
         }, {},
      }, nullptr);
    }
    bank.appendChild(preset, nullptr);
  }
  banks.appendChild(bank, nullptr);
#if JUCE_DEBUG
//    unique_ptr<XmlElement> xml{valueTreeState.state.createXml()};
//    Logger::outputDebugString(xml->createDocument("",false,false));
#endif
}