#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "VTSParamComponent.h"

class YmmySlider: public Component
{
public:
  typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

  YmmySlider(AudioProcessorValueTreeState& valueTreeState, const String& parameterId)
      : component(), paramId(parameterId) {
    component.setSliderStyle(juce::Slider::LinearBarVertical);
    component.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    component.setPopupDisplayEnabled(true, false, this);
    component.setValue(1.0);

    attachment = std::make_unique<SliderAttachment>(valueTreeState, parameterId, component);

    addAndMakeVisible (component);
  }

  ~YmmySlider() {
  }

  void resized() {
    Rectangle<int> r (getLocalBounds());
    component.setBounds(r);
  }

public:
  Slider component;
  const String& paramId;
private:
  std::unique_ptr<SliderAttachment> attachment;
};
