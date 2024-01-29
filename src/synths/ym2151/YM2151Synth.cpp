#include "YM2151Synth.h"
#include "../../MidiConstants.h"
#include <random>


YM2151Synth::YM2151Synth(AudioProcessorValueTreeState& valueTreeState)
    : Synth(valueTreeState),
      channelGroup(0) {
//  initialize();

//  vts.state.addListener(this);

//  vts.addParameterListener("selectedGroup", this);
//  vts.addParameterListener("selectedChannel", this);
//  vts.addParameterListener("fs.bank", this);
//  vts.addParameterListener("fs.preset", this);
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.addParameterListener(param, this);
//  }
}

YM2151Synth::~YM2151Synth() {
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.removeParameterListener(param, this);
//  }
//  vts.removeParameterListener("fs.bank", this);
//  vts.removeParameterListener("fs.preset", this);
//  vts.removeParameterListener("selectedGroup", this);
//  vts.removeParameterListener("selectedChannel", this);
//  vts.state.removeListener(this);
}

std::unique_ptr<juce::AudioProcessorParameterGroup> YM2151Synth::createParameterGroup() {
  auto ym2151Params = std::make_unique<juce::AudioProcessorParameterGroup>("ym2151", "YM2151", "|");

//  for (IntParameter p : FluidSynthParam::parameters) {
//    fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(p.id, p.name, p.min, p.max, p.defaultVal));
//  }
  return ym2151Params;
}

const ValueTree YM2151Synth::getInitialChildValueTree() {
  return
      { "ignoredRoot", {},
       {
       }
      };
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
      std::mt19937 engine{std::random_device{}()};
      std::uniform_int_distribution<uint8_t> dist(0, 127);

      interface.write(0x0F, 0x00);

      for (int i = 0; i < 8; ++i) {
        uint8_t randomByte = dist(engine);
        interface.write(0x20 + i, randomByte | 0xC0);
//        interface.write(0x20 + i, 0xFC);
      }

      for (int i = 0; i < 8; ++i) {
        int absNote = m.getNoteNumber();
        int octave = absNote / 12;
        int note = absNote % 12;

        const int noteTable[12] = {0,1,2,4,5,6,8,9,10,12,13,14};

        uint8_t val = ((octave & 7) << 4) | noteTable[note];
        printf("OCT: %d    NOTE: %d  val: %X \n", octave, note, val);
//        uint8_t randomByte = dist(engine);
        //        interface.write(0x28 + i, 0x31 + i);
        interface.write(0x28 + i, val);
      }

      for (int i = 0; i < 8; ++i) {
        //        uint8_t randomByte = dist(engine);
        interface.write(0x30 + i, 0);
      }

      for (int i = 0; i < 8 * 4; ++i) {
//        uint8_t randomByte = dist(engine);
        interface.write(0x40 + i, 0);

//        interface.write(0x40 + i, randomByte);
//        interface.expire_engine_timer();
      }

      for (int i = 0; i < 8 * 4; ++i) {
        uint8_t randomByte = dist(engine);
//        interface.write(0x60 + i, 0x7F);
//        interface.write(0x60 + i, randomByte);
        interface.write(0x60 + i, 0x10);

//        printf("READSTATUS %X\n", interface.read_status());
//        interface.expire_engine_timer();
//        interface.write(0x10, 0xC8);
//        interface.expire_engine_timer();
//        interface.write(0x11, 0x00);
//        interface.expire_engine_timer();
//        interface.write(0x14, 0x15);
//        interface.expire_engine_timer();
//        printf("READSTATUS %X\n", interface.read_status());

//        interface.irq();
        //        interface.update_clocks()
      }
      for (int i = 0; i < 8 * 4; ++i) {
        uint8_t randomByte = dist(engine);
//        interface.write(0x80 + i, 0x00);
//        interface.write(0x80 + i, randomByte);
//        interface.write(0x80 + i, 0xDF);
        interface.write(0x80 + i, 0x1F);
      }
      for (int i = 0; i < 8 * 4; ++i) {
        uint8_t randomByte = dist(engine);
//        interface.write(0xA0 + i, randomByte);
        interface.write(0xA0 + i, 0xDF);
      }
      for (int i = 0; i < 8 * 4; ++i) {
        uint8_t randomByte = dist(engine);
//        interface.write(0xC0 + i, randomByte);
        interface.write(0xC0 + i, 0x0);
      }
      for (int i = 0; i < 8 * 4; ++i) {
        uint8_t randomByte = dist(engine);
//        interface.write(0xE0 + i, 0xFF);
//        interface.write(0xE0 + i, randomByte);
        interface.write(0xE0 + i, 0x3F);
      }

//      interface.write(0x10, 0xC8);
//      interface.expire_engine_timer();
//      interface.write(0x11, 0x00);
//      interface.expire_engine_timer();
//      interface.write(0x14, 0x15);
//      interface.expire_engine_timer();

      for (int i = 0; i < 8; ++i) {
        interface.write(8, 0x78 + i);
        //      interface.write(0x10, 0xC8);
        //      interface.expire_engine_timer();
        //      interface.write(0x11, 0x00);
        //      interface.expire_engine_timer();
        //      interface.write(0x14, 0x15);
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

  buffer.clear();
  auto bufferWritePointers = buffer.getArrayOfWritePointers();
  if (buffer.getNumChannels() < 2) {
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,"Error",
                                     "Expected at least 2 write buffers for stereo playback. Is there a problem with the current audio device?.");
    return;
  }
  float* writeBuffers[2] = {bufferWritePointers[0], bufferWritePointers[1] };

//  interface.write(0x10, 0xC8);
//  interface.expire_engine_timer();
//  interface.write(0x11, 0x00);
//  interface.expire_engine_timer();
//  interface.write(0x14, 0x15);
//  interface.expire_engine_timer();
  interface.generate(writeBuffers, buffer.getNumSamples());
//  std::mt19937 engine{std::random_device{}()};
//  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
//  for (int i=0; i<2; i++) {
//    for (int j=0; j<buffer.getNumSamples(); j++) {
//      writeBuffers[i][j] = dist(engine);
//    }
//  }
}

void YM2151Synth::prepareToPlay(double sampleRate, int samplesPerBlock) {
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

void YM2151Synth::parameterChanged(const String& parameterID, float newValue) {
  DBG("YM2151Synth::parameterChanged() " + parameterID);
}

void YM2151Synth::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                           const Identifier& property) {
}

void YM2151Synth::setControllerValue(int controller, int value) {
}

void YM2151Synth::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {

}

void YM2151Synth::reset() {
}