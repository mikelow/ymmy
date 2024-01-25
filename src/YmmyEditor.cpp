#include "YmmyEditor.h"
#include "YmmyProcessor.h"
#include "synths/SynthComponent.h"
#include "synths/fluidsynth/FluidSynthComponent.h"

YmmyEditor::YmmyEditor(YmmyProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), valueTreeState(vts),
      midiKeyboard(p.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
      channelLabel(),
      incChannelButton(String("+")), decChannelButton(String("-"))
{
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
  midiKeyboard.setMidiChannel(audioProcessor.getSelectedChannel()+1);
  setWantsKeyboardFocus(true);
  addAndMakeVisible(midiKeyboard);

  channelLabel.setText(String(audioProcessor.getSelectedChannel()+1), dontSendNotification);
  channelLabel.setEditable(true);
  channelLabel.setColour(Label::backgroundColourId, Colours::darkblue);
  channelLabel.setJustificationType(Justification::centred);
  channelLabel.onTextChange = [this] {
    auto text = channelLabel.getText();
    if (!text.containsOnly("0123456789")) {
      channelLabel.setText(String(this->audioProcessor.getSelectedChannel()+1), dontSendNotification);
      return;
    }
    int newChannel = channelLabel.getText().getIntValue();
    if (newChannel < 1 || newChannel > 16) {
      channelLabel.setText(String(this->audioProcessor.getSelectedChannel()+1), dontSendNotification);
      return;
    }

    midiKeyboard.setMidiChannel(newChannel);
    this->audioProcessor.setSelectedChannel(newChannel-1);
  };
  addAndMakeVisible(channelLabel);

  addAndMakeVisible(incChannelButton);
  addAndMakeVisible(decChannelButton);
  incChannelButton.addListener(this);
  decChannelButton.addListener(this);

  setCurrentSynth(FluidSynth);

  vts.state.addListener(this);
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

void YmmyEditor::buttonClicked(Button* button) {
  if (button == &incChannelButton) {
    audioProcessor.incrementChannel();
    printf("GET SELECTED CHANNEL %d\n", audioProcessor.getSelectedChannel());
    midiKeyboard.setMidiChannel(audioProcessor.getSelectedChannel()+1);
  } else if (button == &decChannelButton) {
    audioProcessor.decrementChannel();
    printf("GET SELECTED CHANNEL %d\n", audioProcessor.getSelectedChannel());
    midiKeyboard.setMidiChannel(audioProcessor.getSelectedChannel()+1);
  }
}


void YmmyEditor::resized() {
  // sets the position and size of the component with arguments (x, y, width, height)
  //    midiVolume.setBounds (40, 30, 20, getHeight() - 60);

  auto area = getLocalBounds();

  channelLabel.setBounds( 10, 10, 40, 40);
  incChannelButton.setBounds( 50, 10, 40, 40);
  decChannelButton.setBounds( 90, 10, 40, 40);


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

void YmmyEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                               const Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() == StringRef("settings")) {
    if (property == StringRef("selectedChannel")) {
      int newChannel = treeWhosePropertyHasChanged.getProperty("selectedChannel", 1);
      channelLabel.setText(String(newChannel + 1), dontSendNotification);
    }
  }
}