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

//std::map<fluid_midi_control_change, juce::String> controllerToParam = {
//    {VOLUME_MSB, FluidSynthParam::MIDI_VOLUME},
//    {SOUND_CTRL2, FluidSynthParam::FILTER_RESONANCE}, // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
//    {SOUND_CTRL3, FluidSynthParam::RELEASE_RATE}, // MIDI CC 72 Release time
//    {SOUND_CTRL4, FluidSynthParam::ATTACK_RATE}, // MIDI CC 73 Attack time
//    {SOUND_CTRL5, FluidSynthParam::FILTER_CUT_OFF}, // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
//    {SOUND_CTRL6, FluidSynthParam::DECAY_RATE}, // MIDI CC 75 Decay Time
//    {SOUND_CTRL9, FluidSynthParam::HOLD}, // MIDI CC 73 Attack time
//    {SOUND_CTRL10, FluidSynthParam::SUSTAIN_LEVEL} // MIDI CC 79 undefined
//};

std::map<fluid_gen_type, juce::String> controllerToParam = {
//    {VOLUME_MSB, FluidSynthParam::MIDI_VOLUME},
    {GEN_FILTERQ, FluidSynthParam::FILTER_RESONANCE}, // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
    {GEN_VOLENVRELEASE, FluidSynthParam::RELEASE_RATE}, // MIDI CC 72 Release time
    {GEN_VOLENVATTACK, FluidSynthParam::ATTACK_RATE}, // MIDI CC 73 Attack time
    {GEN_FILTERFC, FluidSynthParam::FILTER_CUT_OFF}, // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
    {GEN_VOLENVDECAY, FluidSynthParam::DECAY_RATE}, // MIDI CC 75 Decay Time
    {GEN_VOLENVHOLD, FluidSynthParam::HOLD}, // MIDI CC 73 Attack time
    {GEN_VOLENVSUSTAIN, FluidSynthParam::SUSTAIN_LEVEL} // MIDI CC 79 undefined
};

std::map<juce::String, fluid_gen_type> createInverseMap() {
  std::map<juce::String, fluid_gen_type> inverseMap;
  for (const auto& pair : controllerToParam) {
    inverseMap[pair.second] = pair.first;
  }
  return inverseMap;
}

std::map<juce::String, fluid_gen_type> paramToController = createInverseMap();



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
    : Synth(valueTreeState), settings{nullptr, nullptr}, synth{nullptr, nullptr}, channelGroup(0),
      currentSampleRate(44100), sfont_id(-1) {
  initialize();

  vts.addParameterListener("bank", this);
  vts.addParameterListener("preset", this);
  for (const auto &[param, controller]: paramToController) {
    vts.addParameterListener(param, this);
  }

  vts.state.addListener(this);
}

FluidSynthSynth::~FluidSynthSynth() {
  for (const auto &[param, controller]: paramToController) {
    vts.removeParameterListener(param, this);
  }
  vts.removeParameterListener("bank", this);
  vts.removeParameterListener("preset", this);
  vts.state.removeListener(this);
}

void FluidSynthSynth::initialize() {
  // deactivate all audio drivers in fluidsynth to avoid FL Studio deadlock when initialising CoreAudio
  // after all: we only use fluidsynth to render blocks of audio. it doesn't output to audio driver.
  const char *DRV[] {NULL};
  fluid_audio_driver_register(DRV);

  settings = { new_fluid_settings(), delete_fluid_settings };

  //    fluid_settings_setint(this->settings.get(), "synth.reverb.active", 1);
  //    fluid_settings_setstr(this->settings.get(), "synth.chorus.active", "no");
  fluid_settings_setstr(this->settings.get(), "synth.midi-bank-select", "gs");
  fluid_settings_setint(this->settings.get(), "synth.midi-channels", NUM_CHANNELS);

  // https://sourceforge.net/p/fluidsynth/wiki/FluidSettings/
#if JUCE_DEBUG
  fluid_settings_setint(settings.get(), "synth.verbose", 1);
#endif
//  fluid_settings_setnum(settings.get(), "synth.sample-rate", currentSampleRate);

  synth = { new_fluid_synth(settings.get()), delete_fluid_synth };

  fluid_synth_set_reverb_on(synth.get(), true);
  //    fluid_synth_set_reverb(synth.get(), 0.7, 0.4, 0.5, 0.5);
  fluid_synth_set_reverb(synth.get(), 0.5, 0.8, 0.5, 0.3);

  fluid_sfloader_t *my_sfloader = new_fluid_defsfloader(settings.get());
  fluid_sfloader_set_callbacks(my_sfloader, mem_open, mem_read, mem_seek, mem_tell, mem_close);
  fluid_synth_add_sfloader(synth.get(), my_sfloader);

  fluid_synth_set_sample_rate(synth.get(), currentSampleRate);

  reset();

  //    fluid_synth_reverb_on(synth.get(), -1, true);
  //    fluid_synth_set_reverb(synth.get(), 1.0, 0.3, 50, 1.0);

  // note: fluid_chan.c#fluid_channel_init_ctrl()
  // > Just like panning, a value of 64 indicates no change for sound ctrls
  // --
  // so, advice is to leave everything at 64
  // and yet, I'm finding that default modulators start at MIN,
  // i.e. we are forced to start at 0 and climb from there
  // --
  // let's zero out every audio param that we manage
  for (const auto &[controller, param]: controllerToParam) {
      setControllerValue(static_cast<int>(controller), 0);
  }

  // http://www.synthfont.com/SoundFont_NRPNs.PDF
      float env_amount{20000.0f};


      fluid_mod_t *my_mod = new_fluid_mod();
      fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
      fluid_mod_set_amount(my_mod, 12000);

//      fluid_mod_set_source2(my_mod, FLUID_MOD_NONE, 0);
//      fluid_mod_set_dest(my_mod, GEN_ATTENUATION);
//      fluid_mod_set_amount(my_mod, 960 * 0.4);

      {
        fluid_mod_set_source1(my_mod,
                              SOUND_CTRL4,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVATTACK);
        fluid_mod_set_amount(my_mod, 12000);
        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
      }

      {
        fluid_mod_set_source1(my_mod,
                              SOUND_CTRL9,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVHOLD);
        fluid_mod_set_amount(my_mod, 12000);
        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
      }

      {
        fluid_mod_set_source1(my_mod,
                              SOUND_CTRL6,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVDECAY);
        fluid_mod_set_amount(my_mod, 12000);
        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
      }

      {
        fluid_mod_set_source1(my_mod,
                              SOUND_CTRL3,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_BIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVRELEASE);
        fluid_mod_set_amount(my_mod, 12000);
        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
      }

//      {
//        fluid_mod_set_source1(my_mod,
//                              SOUND_CTRL10,
//                              FLUID_MOD_CC | FLUID_MOD_CONVEX | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
//        fluid_mod_set_dest(my_mod, GEN_VOLENVSUSTAIN);
//        fluid_mod_set_amount(my_mod, 1000/*cB*/);
//        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
//      }
      {
        fluid_mod_set_source1(my_mod,
                              SOUND_CTRL10,
                              FLUID_MOD_CC | FLUID_MOD_LINEAR | FLUID_MOD_UNIPOLAR | FLUID_MOD_POSITIVE);
        fluid_mod_set_dest(my_mod, GEN_VOLENVSUSTAIN);
        fluid_mod_set_amount(my_mod, 1440/*cB*/);
        fluid_synth_add_default_mod(synth.get(), my_mod, FLUID_SYNTH_OVERWRITE);
      }

//      std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)> mod{new_fluid_mod(), delete_fluid_mod};
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL2), // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
//                            FLUID_MOD_CC
//                            | FLUID_MOD_UNIPOLAR
//                            | FLUID_MOD_CONCAVE
//                            | FLUID_MOD_POSITIVE);
//      fluid_mod_set_source2(mod.get(), 0, 0);
//      fluid_mod_set_dest(mod.get(), GEN_FILTERQ);
//      fluid_mod_set_amount(mod.get(), FLUID_PEAK_ATTENUATION);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//
//      mod = {new_fluid_mod(), delete_fluid_mod};
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL3), // MIDI CC 72 Release time
//                            FLUID_MOD_CC
//                            | FLUID_MOD_UNIPOLAR
//                            | FLUID_MOD_LINEAR
//                            | FLUID_MOD_POSITIVE);
//      fluid_mod_set_source2(mod.get(), 0, 0);
//      fluid_mod_set_dest(mod.get(), GEN_VOLENVRELEASE);
//      fluid_mod_set_amount(mod.get(), env_amount);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//
//      mod = {new_fluid_mod(), delete_fluid_mod};
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL4), // MIDI CC 73 Attack time
//                            FLUID_MOD_CC
//                            | FLUID_MOD_UNIPOLAR
//                            | FLUID_MOD_LINEAR
//                            | FLUID_MOD_POSITIVE);
//      fluid_mod_set_source2(mod.get(), 0, 0);
//      fluid_mod_set_dest(mod.get(), GEN_VOLENVATTACK);
//      fluid_mod_set_amount(mod.get(), env_amount);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//
//      // soundfont spec says that if cutoff is >20kHz and resonance Q is 0, then no filtering occurs
//      mod = {new_fluid_mod(), delete_fluid_mod};
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL5), // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
//                            FLUID_MOD_CC
//                            | FLUID_MOD_LINEAR
//                            | FLUID_MOD_UNIPOLAR
//                            | FLUID_MOD_POSITIVE);
//          fluid_mod_set_source2(mod.get(), 0, 0);
//      fluid_mod_set_dest(mod.get(), GEN_FILTERFC);
//      fluid_mod_set_amount(mod.get(), -2400.0f);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//
//      mod = {new_fluid_mod(), delete_fluid_mod};
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL6), // MIDI CC 75 Decay Time
//                            FLUID_MOD_CC
//                            | FLUID_MOD_UNIPOLAR
//                            | FLUID_MOD_LINEAR
//                            | FLUID_MOD_POSITIVE);
//      fluid_mod_set_source2(mod.get(), 0, 0);
//      fluid_mod_set_dest(mod.get(), GEN_VOLENVDECAY);
//      fluid_mod_set_amount(mod.get(), env_amount);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//
//      mod = {new_fluid_mod(), delete_fluid_mod};
////      fluid_mod_set_source1(mod.get(),
////                            static_cast<int>(SOUND_CTRL10), // MIDI CC 79 undefined
////                            FLUID_MOD_CC
////                            | FLUID_MOD_UNIPOLAR
////                            | FLUID_MOD_LINEAR
//////                            | FLUID_MOD_CONCAVE
////                            | FLUID_MOD_POSITIVE);
////      fluid_mod_set_source2(mod.get(), 0, 0);
////      fluid_mod_set_dest(mod.get(), GEN_VOLENVSUSTAIN);
////      // fluice_voice.c#fluid_voice_update_param()
////      // clamps the range to between 0 and 1000, so we'll copy that
////      fluid_mod_set_amount(mod.get(), 1000.0f);
////      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
//      fluid_mod_set_source1(mod.get(),
//                            static_cast<int>(SOUND_CTRL10),
//                            FLUID_MOD_CC | FLUID_MOD_CONVEX | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
//      fluid_mod_set_dest(mod.get(), GEN_VOLENVSUSTAIN);
//      fluid_mod_set_amount(mod.get(), 60/*cB*/);
//      fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_OVERWRITE);

}

std::unique_ptr<juce::AudioProcessorParameterGroup> FluidSynthSynth::createParameterGroup() {
  auto fluidSynthParams = std::make_unique<juce::AudioProcessorParameterGroup>("fluidsynth", "FluidSynth", "|");
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("midiVolume", "MIDI Volume", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("attackRate", "Attack Rate", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("decayRate", "Decay Rate", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("sustainLevel", "Sustain Level", 0.0f, 127.0f, 1.0f));
//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("releaseRate", "Release Rate", 0.0f, 127.0f, 1.0f));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::MIDI_VOLUME, "MIDI Volume", 0, 127, 1));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::ATTACK_RATE, "Attack Time", -12000, 8000, 1));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::HOLD, "Hold Time", -12000, 5000, 1));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::DECAY_RATE, "Decay Time", -12000, 8000, 1));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::SUSTAIN_LEVEL, "Sustain Level", 0, 1440, 1));
  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(FluidSynthParam::RELEASE_RATE, "Release Rate", -12000, 8000, 1));

//  fluidSynthParams->addChild(std::make_unique<juce::AudioParameterFloat>("fs.soundFont", "MIDI Volume", 0.0f, 127.0f, 1.0f));

//  valueTreeState.state.appendChild({ "soundFont", {
//                                                     { "path", "" },
//                                                     { "memfile", var(nullptr, 0) }
//                                                 }, {} }, nullptr);
  // no properties, no subtrees (yet)
//  valueTreeState.state.appendChild({ "banks", {}, {} }, nullptr);
  return fluidSynthParams;
}

void FluidSynthSynth::updateValueTreeState(AudioProcessorValueTreeState& vts) {
  vts.state.appendChild({ "soundFont",
                          {
                            { "path", "" },
                            { "memfile", var(nullptr, 0) }
                          }, {} }, nullptr);
  vts.state.appendChild({ "banks", {}, {} }, nullptr);
}


void FluidSynthSynth::processMidiMessage(MidiMessage& m) {
  auto rawData = m.getRawData();
  if (rawData[0] >= 0xF0) {
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
      fluid_synth_program_change(synth.get(), m.getChannel() - 1 + channelGroupOffset, 31);
      break;
    case KEY_PRESSURE:
      fluid_synth_channel_pressure(synth.get(), m.getChannel() - 1 + channelGroupOffset, m.getChannelPressureValue());
      break;
    case CONTROL_CHANGE:
      if (m.getControllerNumber() == 115) {
        int value = m.getControllerValue();
        uint8_t programNum = value & 0x7F;
        uint8_t bank = (value >> 7) & 0x7F;
        fluid_synth_program_change(synth.get(), m.getChannel() - 1 + channelGroupOffset, programNum);
      } else {
        fluid_synth_cc(synth.get(), m.getChannel() - 1 + channelGroupOffset,
                       m.getControllerNumber(), m.getControllerValue());
      }
      break;
    case PROGRAM_CHANGE: {
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
  if (sampleRateSetting != sampleRate) {
//    fluid_settings_setnum(settings.get(), "synth.sample-rate", currentSampleRate);
//    delete_fluid_audio_driver(adriver);
    // then delete the synth
//    delete_fluid_synth(synth.get());
    // update the sample-rate
    fluid_settings_setnum(settings.get(), "synth.sample-rate", sampleRate);
    // and re-create objects
//    synth = new_fluid_synth(settings.get());
    synth = { new_fluid_synth(settings.get()), delete_fluid_synth };
//    adriver = new_fluid_audio_driver(settings, synth);
  }

//  fluidSynthModel.setSampleRate(static_cast<float>(sampleRate));
}

int FluidSynthSynth::getNumPrograms() {
  return 128;
}

int FluidSynthSynth::getCurrentProgram() {
  RangedAudioParameter *param{vts.getParameter("preset")};
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
  RangedAudioParameter *param{vts.getParameter("bank")};
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
  RangedAudioParameter *param{vts.getParameter("preset")};
  jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
  AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
  // setCurrentProgram() gets invoked from non-message thread.
  // AudioParameterInt#operator= will activate any listeners of audio parameter "preset".
  // This includes TableComponent, who will update its UI.
  // we need to lock the message thread whilst it does that UI update.
  const MessageManagerLock mmLock;
  *castParam = index;
}

const StringArray FluidSynthSynth::programChangeParams{"bank", "preset"};
void FluidSynthSynth::parameterChanged(const String& parameterID, float newValue) {
  DBG("FluidSynthSynth::parameterChanged()");
  if (programChangeParams.contains(parameterID)) {
    int bank, preset;
    {
      RangedAudioParameter *param{vts.getParameter("bank")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      bank = castParam->get();
    }
    {
      RangedAudioParameter *param{vts.getParameter("preset")};
      jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
      AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
      preset = castParam->get();
    }
    int bankOffset{fluid_synth_get_bank_offset(synth.get(), sfont_id)};
    fluid_synth_program_select(
      synth.get(),
      selectedChannel,
      sfont_id,
      static_cast<unsigned int>(bankOffset + bank),
      static_cast<unsigned int>(preset));
  } else if (
      // https://stackoverflow.com/a/55482091/5257399
      auto it{paramToController.find(parameterID)};
      it != end(paramToController)
    ) {
    RangedAudioParameter *param{vts.getParameter(parameterID)};
    jassert(dynamic_cast<AudioParameterInt*>(param) != nullptr);
    AudioParameterInt* castParam{dynamic_cast<AudioParameterInt*>(param)};
    int value{castParam->get()};
    int controllerNumber{static_cast<int>(it->second)};

    fluid_synth_set_gen(synth.get(), selectedChannel, controllerNumber, static_cast<float>(value));
//    fluid_voice_gen_set(synth.get(), controllerNumber, static_cast<float>(value));

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
  }
}

void FluidSynthSynth::setControllerValue(int controller, int value) {
    fluid_synth_cc(synth.get(), selectedChannel, controller, value);
}

void FluidSynthSynth::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {
//  vts.state.removeListener(this);
//  vts.state.addListener(this);

  vts.addParameterListener("bank", this);
  vts.addParameterListener("preset", this);
  for (const auto &[param, controller]: paramToController) {
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

  const char* testString = (char*)sf;

  sfont_id = fluid_synth_sfload(synth.get(), abused_filename, 1);
  // if -1 is returned, that indicates failure
  // refresh regardless of success, if only to clear the table
  refreshBanks();
}

void FluidSynthSynth::reset() {
  fluid_synth_system_reset(synth.get());
}


void FluidSynthSynth::refreshBanks() {
  ValueTree banks{"banks"};
  fluid_sfont_t* sfont{
      sfont_id == -1
          ? nullptr
          : fluid_synth_get_sfont_by_id(synth.get(), sfont_id)
  };
  if (sfont) {
    int greatestEncounteredBank{-1};
    ValueTree bank;

    fluid_sfont_iteration_start(sfont);
    for(fluid_preset_t* preset {fluid_sfont_iteration_next(sfont)};
         preset != nullptr;
         preset = fluid_sfont_iteration_next(sfont)) {
      int bankNum{fluid_preset_get_banknum(preset)};
      if (bankNum > greatestEncounteredBank) {
        if (greatestEncounteredBank > -1) {
          banks.appendChild(bank, nullptr);
        }
        bank = { "bank", {
                            { "num", bankNum }
                        } };
        greatestEncounteredBank = bankNum;
      }
      bank.appendChild({ "preset", {
                                      { "num", fluid_preset_get_num(preset) },
                                      { "name", String{fluid_preset_get_name(preset)} }
                                  }, {} }, nullptr);
    }
    if (greatestEncounteredBank > -1) {
      banks.appendChild(bank, nullptr);
    }
  }
  vts.state.getChildWithName("banks").copyPropertiesAndChildrenFrom(banks, nullptr);
  vts.state.getChildWithName("banks").sendPropertyChangeMessage("synthetic");

#if JUCE_DEBUG
//    unique_ptr<XmlElement> xml{valueTreeState.state.createXml()};
//    Logger::outputDebugString(xml->createDocument("",false,false));
#endif
}