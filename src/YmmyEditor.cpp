#include "YmmyEditor.h"
#include "YmmyProcessor.h"
#include "synths/SynthComponent.h"
#include "synths/fluidsynth/FluidSynthComponent.h"

YmmyEditor::YmmyEditor(YmmyProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), valueTreeState(vts),
      midiKeyboard(p.keyboardState, MidiKeyboardComponent::horizontalKeyboard) {
  // This is where our plugin’s editor size is set.
  setSize(960, 540);
  //
  //    // these define the parameters of our component object
  //    midiVolume.setSliderStyle (juce::Slider::LinearBarVertical);
  //    midiVolume.setRange (0.0, 127.0, 1.0);
  //    midiVolume.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
  //    midiVolume.setPopupDisplayEnabled (true, false, this);
  //    midiVolume.setTextValueSuffix (" Volume");
  //    midiVolume.setValue(1.0);
  //
  //    // this function adds the component to the editor
  //    addAndMakeVisible (&midiVolume);
  //
  //    // add the listener to the component
  //    midiVolume.addListener (this);

  midiKeyboard.setName ("MIDI Keyboard");
  midiKeyboard.setWantsKeyboardFocus(false);
  setWantsKeyboardFocus(true);
  addAndMakeVisible(midiKeyboard);

  setCurrentSynth(FluidSynth);
}

YmmyEditor::~YmmyEditor() {
}

void YmmyEditor::setCurrentSynth(SynthType synthType) {
  // Remove the current synth UI component if it exists
  if (currentSynth != nullptr) {
    removeChildComponent(currentSynth.get());
  }

  // Create and add the new synth UI based on the selected type
  switch (synthType) {
    case SynthType::FluidSynth:
      currentSynth = std::make_unique<FluidSynthComponent>(valueTreeState);
      break;
    case SynthType::YM2151:
      break;
  }

  addAndMakeVisible(currentSynth.get());
  //    currentSynth->setSize(300, 200);
  resized();  // Call resized to update the layout
}

void YmmyEditor::sliderValueChanged(juce::Slider* slider) {
  audioProcessor.noteOnVel = midiVolume.getValue();
}

void YmmyEditor::resized() {
  // sets the position and size of the component with arguments (x, y, width, height)
  //    midiVolume.setBounds (40, 30, 20, getHeight() - 60);

  auto area = getLocalBounds();
  auto synth = currentSynth.get();
  if (synth) {
    //      synth->setBounds(area.removeFromTop(200));
    synth->setBounds(0, headerHeight, area.getWidth(), area.getHeight() - headerHeight - keyboardHeight);
//    synth->setBounds(area.removeFromBottom(area.getHeight() - headerHeight - keyboardHeight));
  }
  midiKeyboard.setBounds(0, getHeight()-keyboardHeight, getWidth(), keyboardHeight);
}

void YmmyEditor::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::white);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}