#include "YM2151Synth.h"
#include "../../YmmyProcessor.h"
#include "../../MidiConstants.h"
#include <random>

// The clock rate used by CPS1. We'll need to make this dynamic when we support systems that run at 4mhz.
constexpr int CPS1_YM2151_CLOCK_RATE = 3579545;

namespace YM2151SynthParam {
//  const juce::String FILTER_RESONANCE = "fs.filterResonance";
//  const juce::String FILTER_CUT_OFF = "fs.filterCutOff";
//  const juce::String ATTACK_RATE = "fs.attackRate";
//  const juce::String HOLD = "fs.hold";
//  const juce::String DECAY_RATE = "fs.decayRate";
//  const juce::String RELEASE_RATE = "fs.releaseRate";

  IntParameter parameters[] = {
      {"ym2151.bank", "selected bank", 0, 128, 0},
      {"ym2151.preset", "selected preset", 0, 127, 0},
//      {FluidSynthParam::ATTACK_RATE, "Attack Time", -12000, 8000, -12000},
//      {FluidSynthParam::HOLD, "Hold Time", -12000, 5000, -12000},
//      {FluidSynthParam::DECAY_RATE, "Decay Time", -12000, 8000, -12000},
//      {FluidSynthParam::SUSTAIN_LEVEL, "Sustain Level", 0, 1440, 0},
//      {FluidSynthParam::RELEASE_RATE, "Release Rate", -12000, 8000, -12000},
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

void YM2151Synth::receiveFile(juce::MemoryBlock&, SynthFileType fileType) {

}

void YM2151Synth::processMidiMessage(MidiMessage& m) {
  auto rawData = m.getRawData();
  if (rawData[0] >= 0xF0) {
    return;
  }
  const uint8_t eventNibble = rawData[0] & 0xf0;
  int channelGroupOffset = channelGroup * 16;

  switch (eventNibble) {
    case NOTE_OFF:
      for (int i = 0; i < 8; ++i) {
        interface.write(8, i);
      }
      break;
    case NOTE_ON: {
//      std::mt19937 engine{std::random_device{}()};
//      std::uniform_int_distribution<uint8_t> dist(0, 127);

//      interface.write(0x0F, 0x00);

//      for (int i = 0; i < 8; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0x20 + i, randomByte | 0xC0);
//      }

      for (int i = 0; i < 1; ++i) {
        int absNote = m.getNoteNumber();
        // temporary adjustment to get notes aligned with 3.58mhz freq
        absNote -= 13;
        int octave = absNote / 12;
        int note = absNote % 12;

        const int noteTable[12] = {0,1,2,4,5,6,8,9,10,12,13,14};

        uint8_t val = ((octave & 7) << 4) | noteTable[note];
        printf("OCT: %d    NOTE: %d  val: %X \n", octave, note, val);
        interface.write(0x28 + i, val);
      }
//
//      for (int i = 0; i < 8; ++i) {
//        interface.write(0x30 + i, 0);
//      }
//
//      for (int i = 0; i < 8 * 4; ++i) {
//        interface.write(0x40 + i, 0);
//      }
//
//      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0x60 + i, 0x10);
////        interface.write(0x60 + i, 0x7f);
////        interface.write(0x60 + i, randomByte);
//      }
//      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0x80 + i, 0x1F);
//        interface.write(0x80 + i, 0x1F);
//      }
//      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0xA0 + i, 0xDF);
//      }
//      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0xC0 + i, 0x0);
//      }
//      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
//        interface.write(0xE0 + i, 0x3F);
//      }

      for (int i = 0; i < 1; ++i) {
        interface.write(8, 0x78 + i);
      }
      break;
    }
    case KEY_PRESSURE:
      break;
    case CONTROL_CHANGE:
      break;
    case PROGRAM_CHANGE:
      break;
    case CHANNEL_PRESSURE:
      break;
    case PITCH_BEND:
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
    interface.changePreset(patch, selectedChannel);
//    fluid_synth_program_select(
//        synth.get(),
//        selectedChannel + (selectedGroup*16),
//        sfont_id,
//        static_cast<unsigned int>(bankOffset + bank),
//        static_cast<unsigned int>(preset));
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
}

void YM2151Synth::refreshBanks(std::vector<OPMPatch>& patches) {
  ValueTree banks = ValueTree("ym2151.banks");

//  fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(synth.get(), sfont_id);
//  if (!sfont) {
//    return;
//  }

//  int greatestEncounteredBank = -1;
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
//    preset.appendChild({ "LFO", {
//      { "LFRQ", patch.lfoParams.LFRQ },
//      { "AMD", patch.lfoParams.AMD },
//      { "PMD", patch.lfoParams.PMD },
//      { "WF", patch.lfoParams.WF },
//      { "NFRQ", patch.lfoParams.NFRQ },
//    }, {} }, nullptr);
//    bank.appendChild({ "preset", {
//      { "num", patch.number },
//      { "name", String(patch.name) },
//    }, {} }, nullptr);
    bank.appendChild(preset, nullptr);
  }
  banks.appendChild(bank, nullptr);

//  fluid_sfont_iteration_start(sfont);
//  for(fluid_preset_t* preset = fluid_sfont_iteration_next(sfont);
//       preset != nullptr;
//       preset = fluid_sfont_iteration_next(sfont)) {
//    int bankNum = fluid_preset_get_banknum(preset);
//    if (bankNum > greatestEncounteredBank) {
//      if (greatestEncounteredBank > -1) {
//        banks.appendChild(bank, nullptr);
//      }
//      bank = { "fs.bank", { { "num", bankNum } } };
//      greatestEncounteredBank = bankNum;
//    }
//    bank.appendChild({ "fs.preset", {
//                                       { "num", fluid_preset_get_num(preset) },
//                                       { "name", String(fluid_preset_get_name(preset)) }
//                                   }, {} }, nullptr);
//  }
//  if (greatestEncounteredBank > -1) {
//    banks.appendChild(bank, nullptr);
//  }
  vts.state.getChildWithName("ym2151.banks").copyPropertiesAndChildrenFrom(banks, nullptr);
  vts.state.getChildWithName("ym2151.banks").sendPropertyChangeMessage("synthetic");

#if JUCE_DEBUG
//    unique_ptr<XmlElement> xml{valueTreeState.state.createXml()};
//    Logger::outputDebugString(xml->createDocument("",false,false));
#endif
}