#include "YM2151Synth.h"
#include "../../YmmyProcessor.h"
#include "../../MidiConstants.h"
#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>

// The clock rate used by CPS1. We'll need to make this dynamic when we support systems that run at 4mhz.
constexpr int CPS1_YM2151_CLOCK_RATE = 3579545;


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

namespace {
  juce::String serializeDriverData(const std::vector<uint8_t>& data) {
    StringArray parts;
    for (auto byte : data) {
      parts.add(String(static_cast<int>(byte)));
    }
    return parts.joinIntoString(" ");
  }

  std::vector<uint8_t> parseDriverData(const juce::String& data) {
    std::vector<uint8_t> parsed;
    std::istringstream iss(data.toStdString());
    int value = 0;
    while (iss >> value) {
      if (value >= 0 && value <= 0xFF) {
        parsed.push_back(static_cast<uint8_t>(value));
      }
    }
    return parsed;
  }
}


namespace {
  std::string toLowerCopy(const std::string& value) {
    std::string lowered(value.size(), '\0');
    std::transform(value.begin(), value.end(), lowered.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowered;
  }
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

void YM2151Synth::changePreset(OPMPatch& patch, int channel) {
  switchDriverIfNeeded(patch.driver.name, channel);
  auto& chState = chanState[channel];
  chState.ym.updateWithPatch(patch);

  // ensureCurrentDriver();

  if (currentDriver->enableLFO(channel)) {
    globState.ym.LFRQ = patch.lfoParams.LFRQ;
    globState.ym.AMD = patch.lfoParams.AMD;
    globState.ym.PMD = patch.lfoParams.PMD;
    globState.ym.WF = patch.lfoParams.WF;
  }
  interface.changePreset(patch, channel, currentDriver->enableLFO(channel));

  currentDriver->assignPatchToChannel(patch, channel, *this, chState);
}

void YM2151Synth::defaultCCHandler(int channel, uint8_t controllerNum) {
  ChannelState& chState = chanState[channel];
  uint8_t controllerValue = chState.midi.cc[controllerNum];

  switch (controllerNum) {
    case BANK_SELECT_MSB:
      chState.midi.bankMsb = controllerValue;
      break;

    case BANK_SELECT_LSB:
      chState.midi.bankLsb = controllerValue;
      break;

    // VOPM implementation:
    case MODULATION_MSB:    // CC 1
    case EFFECTS1_MSB:      // CC 12
      setAMD(controllerValue);
      break;

    case BREATH_MSB:        // CC 2
    case EFFECTS2_MSB:      // CC 13
      setPMD(controllerValue);
      break;

    case 3:                 // CC 3
      setLFRQ((controllerValue << 1) | chState.midi.lfrqLo );
      break;

    case MODULATION_WHEEL_LSB: // CC 33
    case EFFECTS1_LSB:         // CC 44
      // AMD lo
      break;

    case BREATH_LSB:        // CC 34
    case EFFECTS2_LSB:      // CC 45
      // PMD lo
      break;

    case SOUND_CTRL8:       // CC 77 Sound Controller 8 (Vibrato Depth)
      // PMS
      setPMS(channel, controllerValue / 16);
      break;

    case EFFECTS_DEPTH2:    // CC 92 Effects 2 Depth (Tremolo Depth)
      // AMS
      setAMS(channel, controllerValue / 32);
      break;

    case 35:
      chState.midi.lfrqLo = controllerValue & 1;
      break;

    case VOLUME_MSB: {
      chState.midi.volume = controllerValue;
      defaultChannelTLUpdate(channel);
      break;
    }
    case PAN_MSB: {
      chState.midi.pan = controllerValue;
      defaultChannelPanUpdate(channel);
      break;
    }

    case 79: {
      setWaveform(controllerValue);
      break;
    }
  }
}

void YM2151Synth::defaultChannelTLUpdate(int channel) {
  ChannelState& chState = chanState[channel];

  // if (!chState.isNoteActive) {
  //   return;
  // }

  uint8_t CON_limits[8] = { 1, 1, 1, 1, 2, 3, 3, 4 };
  std::array<uint8_t, 4> opAtten{};

  auto conLimit = CON_limits[chState.ym.CON];
  for (int i = 0; i < 4; i++) {
    int slot = 3 - i;
    int16_t tl = chState.ym.TL[slot];
    if (i < conLimit) {
      tl += (0x7F - chState.midi.volume);
      tl += (0x7F - chState.midi.velocity);
    }
    tl = std::clamp<int16_t>(tl, 0, 127);
    opAtten[slot] = tl;
  }
  setChannelTL(channel, opAtten);
}

void YM2151Synth::defaultChannelPanUpdate(int channel) {
  uint8_t pan = chanState[channel].midi.pan;

  if (pan == 0) {
    setChannelRL(channel, RLSetting::L);
  } else if (pan == 0x7F) {
    setChannelRL(channel, RLSetting::R);
  } else {
    setChannelRL(channel, RLSetting::RL);
  }
}


void YM2151Synth::setChannelTL(int channel, const std::array<uint8_t, 4>& atten) {
  for (int i = 0; i < 4; i++) {
    interface.write(0x60 + (i*8) + channel, atten[i]);
  }
}

void YM2151Synth::setChannelRL(int channel, RLSetting rl) {
  assert(static_cast<uint8_t>(rl) < 4);
  auto& ymChState = chanState[channel].ym;

  uint8_t rlVal = static_cast<uint8_t>(rl);
  ymChState.RL = rlVal;
  uint8_t dataByte = (rlVal << 6) | (ymChState.FL << 3) | ymChState.CON;
  interface.write(0x20 + channel, dataByte);
}

void YM2151Synth::setLFRQ(uint8_t lfrq) {
  globState.ym.LFRQ = lfrq;
  interface.write(0x18, lfrq);
}

void YM2151Synth::setAMD(uint8_t amd) {
  assert(amd < 0x80);
  amd &= 0x7F;
  globState.ym.AMD = amd;
  interface.write(0x19, amd & 0x7F);
}

void YM2151Synth::setPMD(uint8_t pmd) {
  assert(pmd < 0x80);
  pmd &= 0x7F;
  globState.ym.PMD = pmd;
  interface.write(0x19, pmd | 0x80);
}

void YM2151Synth::setWaveform(uint8_t wf) {
  assert(wf < 4);
  wf &= 3;
  globState.ym.WF = wf;
  interface.write(0x1B, wf & 3);
}

void YM2151Synth::setAMS(int channel, uint8_t ams) {
  assert(channel >= 0 && channel < 8);
  assert(ams < 4);
  ams &= 3;
  chanState[channel].ym.AMS = ams;
  uint8_t newPmsAms = (chanState[channel].ym.PMS << 4) | ams;
  interface.write(0x38 + channel, newPmsAms);
}

void YM2151Synth::setPMS(int channel, uint8_t pms) {
  assert(channel >= 0 && channel < 8);
  assert(pms < 8);
  pms &= 7;
  chanState[channel].ym.PMS = pms;
  uint8_t newPmsAms = chanState[channel].ym.AMS | (pms << 4);
  interface.write(0x38 + channel, newPmsAms);
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
  auto& chState = chanState[channel];
  // YM2151ChannelState& ymChState = ym2151ChannelState[channel];
  // MidiChannelState& midiChState = ym2151ChannelState[channel];

  switch (eventNibble) {
    case NOTE_OFF:
      chState.midi.isNoteActive = false;
      interface.write(8, channel);
      break;
    case NOTE_ON: {
      int absNote = m.getNoteNumber();
      int vel = m.getVelocity();

      // temporary adjustment to get notes aligned with 3.58mhz freq
      absNote -= 13;
      if ((chState.ym.KF & 0x20) > 0) {     // if KF high bit is set
        absNote -= 1;
      }
      int octave = absNote / 12;
      int note = absNote % 12;

      const int noteTable[12] = {0,1,2,4,5,6,8,9,10,12,13,14};

      uint8_t finalNoteValue = ((octave & 7) << 4) | noteTable[note];

      // Set volume of note
      chState.midi.isNoteActive = true;
      chState.midi.note = finalNoteValue;
      chState.midi.velocity = vel;
      if (!currentDriver->updateChannelTL(channel, *this, chState)) {
        defaultChannelTLUpdate(channel);
      }

      // printf("OCT: %d    NOTE: %d  val: %X  atten: %X  ksAtten: %X \n", octave, note,
      // finalNoteValue);
      // Send octave / note
      interface.write(0x28 + channel, finalNoteValue);
      // Send note on
      interface.write(8, (chState.ym.SLOT_MASK << 3) | channel);

      // If the instrument reset_lfo flag is set, we reset LFO phase after note on
      if (currentDriver->shouldResetLFOOnNoteOn(channel)) {
        interface.write(1, 2);
        interface.write(1, 0);
      }
      break;
    }
    case KEY_PRESSURE:
      break;
    case CONTROL_CHANGE: {
      int controllerNum = m.getControllerNumber();
      int controllerValue = m.getControllerValue();
      if (controllerNum >= 0 && controllerNum <= 127 && controllerValue >= 0 && controllerValue <= 127) {
        chState.midi.cc[m.getControllerNumber()] = m.getControllerValue();
      }
      if (!currentDriver->handleCC(channel, controllerNum, *this, chState)) {
        defaultCCHandler(channel, controllerNum);
      }
      break;
    }
    case PROGRAM_CHANGE: {
      auto patch = loadPresetFromVST(chState.midi.bankMsb, m.getProgramChangeNumber());
      changePreset(patch, channel);
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
      chState.ym.KF = kf;
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
  int time;
  MidiMessage m;

  auto bufferWritePointers = buffer.getArrayOfWritePointers();
  if (buffer.getNumChannels() < 2) {
    DBG("Error: Expected at least 2 write buffers for stereo playback.");
    return;
  }

  auto nativeBufferWritePointers = nativeBuffer->getArrayOfWritePointers();
  float* writeBuffers[2] = {nativeBufferWritePointers[0], nativeBufferWritePointers[1]};

  int numNativeSamples = nativeBuffer->getNumSamples();
  double ratio = static_cast<double>(numNativeSamples) / processor->getSampleRate();

  int currentTime = 0;
  int bufferOffset = 0;
  float samplesRemainder = 0;
  for (MidiBuffer::Iterator i{midiMessages}; i.getNextEvent(m, time);) {
    if (time > currentTime) {
      float samplesElapsed = ((time - currentTime) * ratio) + samplesRemainder;
      float samplesToGenerate;
      samplesRemainder = std::modf(samplesElapsed, &samplesToGenerate);

      interface.generate(writeBuffers, bufferOffset, samplesToGenerate);
      bufferOffset += samplesToGenerate;
      currentTime = time;
    }
    processMidiMessage(m);
  }

  const int numSamples = buffer.getNumSamples();
  int remainingSamplesToGenerate = ceil(numSamples * ratio) - bufferOffset;
  interface.generate(writeBuffers, bufferOffset, remainingSamplesToGenerate);

  // Mix generated samples with existing content in the buffer
  for (int channel = 0; channel < 2; ++channel) {
    float* output = bufferWritePointers[channel];
    tempBuffer.setSize(1, numSamples, false, false, true);

    // Use the resampler to write into the temporary buffer
    resamplers[channel].process(ratio, nativeBufferWritePointers[channel], tempBuffer.getWritePointer(0), numSamples);

    // Add the contents of the tempBuffer to the output buffer using SIMD operations
    juce::FloatVectorOperations::add(output, tempBuffer.getReadPointer(0), numSamples);
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
    changePreset(patch, selectedChannel);
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

  auto driver = preset.getChildWithName("DRIVER");
  if (driver.isValid()) {
    patch.driver.name = driver.getProperty("name").toString().toStdString();
    patch.driver.dataBytes = parseDriverData(driver.getProperty("data").toString());
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
  globState.reset();
  for (int i = 0; i < 8; i++) {
    interface.resetChannel(i);
    chanState[i].reset();
  }
  currentDriver = createDriver("default");
  activeDriverKey = normalizeDriverName("default");
}

std::string YM2151Synth::normalizeDriverName(const std::string& name) const {
  auto normalized = toLowerCopy(name);
  if (normalized.empty()) {
    return "default";
  }
  return normalized;
}

void YM2151Synth::ensureCurrentDriver() {
  if (currentDriver) {
    return;
  }

  auto requestedKey = activeDriverKey.empty() ? normalizeDriverName("") : activeDriverKey;
  currentDriver = createDriver(requestedKey);
  activeDriverKey = normalizeDriverName(requestedKey);
}

void YM2151Synth::resetChannelsExcept(int preservedChannel) {
  for (int i = 0; i < 8; ++i) {
    if (i == preservedChannel) {
      continue;
    }
    interface.resetChannel(i);
    chanState[i].reset();
  }
}

void YM2151Synth::switchDriverIfNeeded(const std::string& desiredDriverName, int targetChannel) {
  auto desiredKey = normalizeDriverName(desiredDriverName);
  const bool needsSwitch = !currentDriver || activeDriverKey != desiredKey;
  if (!needsSwitch) {
    return;
  }

  resetChannelsExcept(targetChannel);

  currentDriver = createDriver(desiredDriverName);
  if (!currentDriver) {
    currentDriver = createDriver("");
    desiredKey = normalizeDriverName("");
  }
  activeDriverKey = desiredKey;
}

void YM2151Synth::refreshBanks(std::vector<OPMPatch>& patches) {
  ValueTree banks = vts.state.getOrCreateChildWithName("ym2151.banks", nullptr); // Access the existing tree or create it

  banks.removeAllChildren(nullptr); // Clear any previous banks if needed

  ValueTree bank;

  int numPatches = patches.size();
  int numBanks = (numPatches / 128) + 1;
  for (int b = 0; b < numBanks; ++b) {
    bank = { "bank", { { "num", b }}};
    for (int i = 0; i < 128; ++i) {
      int p = b*128 + i;
      if (p >= numPatches)
        break;
      auto patch = patches[p];

      ValueTree preset = { "preset", {
          { "num", i },
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
      if (!patch.driver.name.empty()) {
        preset.appendChild({ "DRIVER", {
              { "name", String(patch.driver.name) },
              { "data", serializeDriverData(patch.driver.dataBytes) },
             }, {},
          }, nullptr);
      }
      bank.appendChild(preset, nullptr);
    }
    banks.appendChild(bank, nullptr);
  }

#if JUCE_DEBUG
//    unique_ptr<XmlElement> xml{valueTreeState.state.createXml()};
//    Logger::outputDebugString(xml->createDocument("",false,false));
#endif
}